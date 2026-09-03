# TLPI 第 04 章 — File I/O: The Universal I/O Model

**优先级**：🟡→🔴（嵌入式 / HFT 文件与设备 I/O 地基）  
**前置**：Ch2 基本概念（fd）· Ch3 系统编程概念（错误处理等）  
**后置**：书内 Ch5 Further Details → [`../chapter-05-file-io-further/`](../chapter-05-file-io-further/)

---

## 小节目录

- [4.1 Overview 概览](notes/4.1-overview.md)
- [4.2 通用 I/O 模型（核心思想）](notes/4.2-universality.md)
- [4.3 `open()`](notes/4.3-open.md)
- [4.4 `read()`](notes/4.4-read.md)
- [4.5 `write()`](notes/4.5-write.md)
- [4.6 `close()`](notes/4.6-close.md)
- [4.7 `lseek()`](notes/4.7-lseek.md)
- [4.8 `ioctl()`](notes/4.8-ioctl.md)
- [4.9 Summary](notes/4.9-summary.md)
- [4.10 Exercises](notes/4.10-exercises.md)

---

## 章节定位


UNIX「一切皆文件」的操作根基：**同一套 4 个系统调用** 操作普通文件、终端、管道、socket、设备文件。

```
open() → read() / write() → close()
（+ lseek / ioctl 视对象而定）
```

---


---

## 示例：通用拷贝（Listing 4-1 精神）


见 [`code/copy.c`](./code/copy.c)（不依赖书中 `tlpi_hdr.h`）。

```bash
cc -Wall -o copy code/copy.c
./copy a.txt b.txt          # 文件 → 文件
./copy a.txt /dev/tty       # 文件 → 终端
./copy /dev/tty log.txt     # 键盘 → 文件（Ctrl+D 结束）
```

---


---

## 易错清单


1. **fd** 在进程 fd 表；**偏移** 在打开文件描述（open file description）— Ch5 三层结构。  
2. `close(fd)` 释放槽位；进程退出自动关全部 fd。  
3. 管道/socket **不要**假设可 `lseek`。  
4. 短读、部分写是 **正常现象**，必须处理。  
5. `umask` 影响新建权限；`mode` 仅配合 `O_CREAT`。  
6. `write` 成功 ≠ 落盘。

---


---

## 双线提示


| 路线 | 带走 |
|------|------|
| 嵌入式 | `/dev` 设备也走同一套 open/read/write；专属控制靠 ioctl |
| HFT | 短读/部分写；热路径少 syscall；落盘语义与缓冲要清楚 |

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 通用模型：`open/read/write/close`（+lseek/ioctl） |
| 2 | fd 0/1/2；进程私有 |
| 3 | 短读/部分写合法；循环处理 |
| 4 | lseek 改 offset；管道 ESPIPE |
| 5 | 书内 Ch4=本章；Ch3=系统编程概念 |

---


---

## 参考


- 《The Linux Programming Interface》**第 04 章** — File I/O: The Universal I/O Model  
- [OUTLINE](../OUTLINE.md) · 下一内容：书内 Ch5 → [`../chapter-05-file-io-further/`](../chapter-05-file-io-further/)


---

## 代码示例

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

/* Ch4 通用 I/O 模型 — open/read/write/close 四件套。
 * 所有文件类型（普通文件/设备/proc）统一用这套接口。
 * 编译: gcc -o ch4_demo ch4_demo.c */

int main(void) {
    int fd = open("/tmp/ch4_test.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open"); return 1; }

    const char *msg = "Hello, Universal I/O!\n";
    write(fd, msg, strlen(msg));
    close(fd);

    /* 重新打开读取 */
    fd = open("/tmp/ch4_test.txt", O_RDONLY);
    if (fd < 0) { perror("open read"); return 1; }

    char buf[256];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        printf("read back: %s", buf);
    }
    close(fd);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
