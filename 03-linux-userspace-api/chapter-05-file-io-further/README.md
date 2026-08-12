# TLPI 第 05 章 — File I/O: Further Details

**优先级**：🟡→🔴（HFT：非阻塞、`pread`/`pwrite`、偏移共享）  
**前置**：[Ch4 Universal I/O](../chapter-04-file-io-universal/notes.md)  
**后置**：[Ch6 Processes](../chapter-06-processes/notes.md) · [Ch13 File I/O Buffering](../chapter-13-file-io-buffering/notes.md)  

---

## 小节目录

- [5.1 内核三层结构（本章核心 · 必考）](./notes/5.1-structure.md)
- [5.2 `dup` / `dup2` / `dup3`](./notes/5.2-dup-dup2-dup3.md)
- [5.3 `pread` / `pwrite`](./notes/5.3-pread-pwrite.md)
- [5.4 原子操作](./notes/5.4-atomic-operations.md)
- [5.5 `fcntl` — 文件控制](./notes/5.5-fcntl.md)
- [5.6 打开标志补充（扩展 Ch4）](./notes/5.6-ch4.md)
- [5.7 非阻塞 `O_NONBLOCK`](./notes/5.7-ononblock.md)
- [5.8 大文件 LFS](./notes/5.8-lfs.md)
- [5.9 `/dev/fd`（Linux）](./notes/5.9-dev-fd.md)

---

## 章节目标


揭示 Linux 内核 **三层文件结构**，理解：fd 复制、原子操作、`pread`/`pwrite`、`fcntl`、非阻塞 I/O、大文件支持。

| | |
|--|--|
| **Ch4** | 会用 `open/read/write/lseek`，只认识 fd **数字** |
| **Ch5** | 看透三层结构；解释偏移共享、多进程写覆盖、fd 重定向 |
| **Ch13** | 页缓存、`write` 缓冲、`fsync`（「write 成功 ≠ 已落盘」） |

---


---

## 易错清单


1. 偏移在 **打开文件描述**；`dup` 共享，独立 `open` 独立。  
2. `FD_CLOEXEC` = fd 标志；`O_APPEND`/`O_NONBLOCK` = 文件状态标志。  
3. `pread` ≠ `lseek`+`read`（原子性 + 不改全局偏移）。  
4. `O_EXCL` 须配 `O_CREAT`。  
5. `fcntl(F_SETFL)` **改不了** 读写模式。  
6. `O_NONBLOCK` 对普通磁盘文件通常无效。

---


---

## Ch4 vs Ch5 速查


| | Ch4 Universal I/O | Ch5 Further Details |
|--|-------------------|---------------------|
| 焦点 | 同一套 API 操作万物 | 内核三层 + 进阶语义 |
| API | `open/read/write/close/lseek` | `dup*`、`pread*`、`fcntl`、原子标志 |
| 偏移 | 「有个游标」 | 游标在 **打开描述**；谁共享谁独立 |
| 非阻塞 | 少提 | `O_NONBLOCK` 适用对象与语义 |
| 落盘 | 未深入 | → Ch13 缓冲 / `fsync` |

---


---

## 章节链路


```
Ch4 会用 fd 数字
  → Ch5 三层结构解释「怪现象」
  → Ch6 进程环境
  → Ch13 缓冲：write 成功 ≠ 落盘（fsync）
```

---


---

## 双线提示


| 路线 | |
|------|--|
| 嵌入式 | `dup2` 重定向；`O_CLOEXEC` 防 exec 泄漏；设备 fd 的 `fcntl` |
| HFT | `pread`/`pwrite` 并发；少 `lseek` 竞态；非阻塞多用于 socket/管道 |

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | fd 表 → 打开描述（偏移+状态）→ inode |
| 2 | 双 open 独立偏移；dup/fork 共享偏移 |
| 3 | `O_APPEND` / `O_CREAT\|O_EXCL` 原子语义 |
| 4 | fd 标志 vs 文件状态标志 |
| 5 | Ch5 = Further Details，不是 Ch4 |

---


---

## 参考


- 《The Linux Programming Interface》**第 05 章** — File I/O: Further Details  
- [OUTLINE](../OUTLINE.md) · [Ch4](../chapter-04-file-io-universal/notes.md) · [Ch13](../chapter-13-file-io-buffering/notes.md) · [LKD §3.8](../../05-linux-kernel/00_Book_3rd_Notes/chapter-03-process-management/notes/section-3.8-身份PID与资源FD.md)


---

## 代码示例

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

/* Ch5 深入文件 I/O — dup/dup2/fcntl/lseek/pread/pwrite。
 * 演示 dup2 重定向 + lseek 随机访问。
 * 编译: gcc -o ch5_demo ch5_demo.c */

int main(void) {
    int fd = open("/tmp/ch5_test.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open"); return 1; }

    /* 写入数据 */
    const char *data = "ABCDEFGHIJ";
    write(fd, data, 10);

    /* lseek 到开头，用 pread 读特定位置（不影响文件偏移量） */
    char buf[4];
    pread(fd, buf, 3, 5);  /* 从偏移5读3字节 */
    buf[3] = '\0';
    printf("pread at offset 5: %s\n", buf);

    /* dup2: 将 fd 复制到 STDOUT_FILENO，重定向输出 */
    lseek(fd, 0, SEEK_SET);
    dup2(fd, STDOUT_FILENO);
    close(fd);

    /* printf 现在写入文件而不是终端 */
    printf("This goes to the file via dup2!\n");
    fflush(stdout);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
