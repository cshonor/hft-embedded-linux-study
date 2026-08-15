# TLPI 第 13 章 — File I/O Buffering

**优先级**：🔴（日志 / 持久化 / 高性能 IO / DB）  
**前置**：[Ch4 Universal I/O](../chapter-04-file-io-universal/README.md) · [Ch5 Further I/O](../chapter-05-file-io-further/README.md) · [Ch12 `/proc`](../chapter-12-system-process-info/README.md)  
**后置**：[Ch14 File Systems](../chapter-14-file-systems/README.md) · [Ch49 mmap](../chapter-49-memory-mappings/README.md) · Ch63 替代 I/O

---

## 小节目录

- [13.1 内核缓冲（Buffer / Page Cache）](./notes/13.1-buffer-page-cache.md)
- [13.2 stdio 用户态缓冲（`FILE*`）](./notes/13.2-stdio-file.md)
- [13.3 【致命】混用 `FILE*` 与 `read`/`write`(fd)](notes/13.3-controlling-kernel-buffering.md)
- [13.4 `posix_fadvise` — 访问模式提示](notes/13.4-summary-buffering.md)
- [13.5 Direct I/O（`O_DIRECT`）](notes/13.5-advising-kernel.md)
- [13.6 两层数据流](notes/13.6-direct-io.md)

---

## 章节目标


厘清两层缓冲与延迟写；掌握 `fsync`/`fdatasync`/`O_SYNC`；会控 stdio 缓冲与 `fflush`；**严禁无保护地混用 stdio 与原生 read/write**；了解 `posix_fadvise` / `O_DIRECT`。

---


---

## 13.7 速查：持久化相关标志/调用


| 机制 | 作用范围 | 等不等落盘 | 备注 |
|------|----------|------------|------|
| `write` | → 页缓存 | 否 | 成功 ≠ 落盘 |
| `fsync` | 一文件数据+元数据 | 是 | 最稳、最贵 |
| `fdatasync` | 优先数据 | 是 | 常够用 |
| `sync` | 全局 | 不定 | 业务禁用狂调 |
| `O_SYNC` | 每次 write≈fsync | 是 | 极慢 |
| `O_DSYNC` | 每次 write≈fdatasync | 是 | |
| `O_DIRECT` | 绕过页缓存 | **否** | 仍要 fsync 才谈持久化 |
| `fflush` | **仅 stdio 用户缓冲** | 否 | 不刷磁盘 |

---


---

## 13.8 易错清单


1. `write` 成功 ≠ 落盘  
2. 只要数据、不要改大小/mtime → 优先 `fdatasync`  
3. stdout→文件/管道：全缓冲，无 `\n`/`fflush` 看不见输出  
4. `O_SYNC` 管的是 `write`，管不住未 `fflush` 的 stdio  
5. `O_DIRECT` 对齐失败 → `EINVAL`；用 `posix_memalign`  
6. 崩溃丢：**用户态 stdio** + **内核脏页**；已刷盘的安全  

---


---

## 练习


1. 终端 vs 重定向：stdout 行缓冲/全缓冲 + `fflush`  
2. 小循环多次 `write` vs 加大缓冲/批写  
3. 复现 `printf`+`write` 乱序，用 `fflush` 修  
4. `O_DIRECT` 故意不对齐 → `EINVAL`  
5. （选）`posix_fadvise` 顺序读提示  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 两层：内核页缓存 + stdio 用户缓冲 |
| 2 | `write` ≠ 落盘；要持久化用 `fsync`/`fdatasync` |
| 3 | 混用 stdio 与 read/write 前必须 `fflush` |
| 4 | 重定向后 stdout 常变全缓冲 |
| 5 | `O_DIRECT` 绕缓存≠持久化；要对齐 |
| 6 | `fdatasync` 通常够；`sync()` 勿滥用 |

---


---

## 参考


- Kerrisk · TLPI Ch13  
- `man 2 fsync` · `man 2 fdatasync` · `man 2 open`（`O_SYNC`/`O_DIRECT`）· `man 3 setvbuf` · `man 3 posix_fadvise`


---

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

/* Ch13 文件 I/O 缓冲 — stdio 缓冲 vs 内核缓冲 vs 无缓冲。
 * 演示 setvbuf 控制用户态缓冲 + O_DIRECT 绕过内核缓冲。
 * 编译: gcc -o ch13_demo ch13_demo.c */

int main(void) {
    /* 行缓冲: 遇到 \n 才 flush */
    char line_buf[1024];
    setvbuf(stdout, line_buf, _IOLBF, sizeof(line_buf));

    printf("This is line-buffered");  /* 无 \n，暂不输出 */
    fflush(stdout);                    /* 手动 flush */
    printf(" - now flushed\n");

    /* 全缓冲 vs 无缓冲对比 */
    FILE *fp = fopen("/tmp/ch13_test.txt", "w");
    setvbuf(fp, NULL, _IOFBF, 8192);  /* 全缓冲 8KB */
    fprintf(fp, "fully buffered\n");
    fclose(fp);  /* close 时 flush */

    /* 内核缓冲: write() 先进内核 page cache，不保证落盘 */
    int fd = open("/tmp/ch13_test2.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    write(fd, "kernel buffered\n", 16);
    fsync(fd);   /* 强制刷盘，绕过内核缓冲延迟 */
    close(fd);

    printf("stdio buffer -> kernel buffer -> disk (fsync)\n");
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
