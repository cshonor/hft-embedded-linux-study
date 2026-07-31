# TLPI 第 04 章 — File I/O: The Universal I/O Model

> **勘误（重要）：** 本书第 **4** 章是「通用 I/O 模型」；第 **3** 章才是 *System Programming Concepts*。  
> 易与 **APUE 第 3 章** 混淆。  
> 仓库目录名 `chapter-03-file-io/` 是历史编号，**内容对应书内第 4 章**。  
> 官方样章：[TLPI-04 PDF](https://www.man7.org/tlpi/download/TLPI-04-File_IO_The_Universal_IO_Model.pdf)

## 学习状态

- [x] 已通读
- [x] 已做笔记
- [ ] C 示例已跑（见 [`code/copy.c`](./code/copy.c)）
- [ ] Rust 对照已写

**优先级**：🟡→🔴（嵌入式 / HFT 文件与设备 I/O 地基）  
**前置**：Ch2 基本概念（fd）· Ch3 系统编程概念（错误处理等）  
**后置**：书内 Ch5 Further Details（`dup`/`fcntl`/原子追加等）→ 目录 `chapter-04-file-unbuffered-io/`

---

## 章节定位

UNIX「一切皆文件」的操作根基：**同一套 4 个系统调用** 操作普通文件、终端、管道、socket、设备文件。

```
open() → read() / write() → close()
（+ lseek / ioctl 视对象而定）
```

---

## 4.2 通用 I/O 模型（核心思想）

| 点 | 说明 |
|----|------|
| 同一套 API | 拷贝磁盘文件与读写 `/dev/tty` 可用同一段代码 |
| 差异在哪 | 设备/文件系统逻辑在 **内核驱动**；应用层无感 |
| 通用不够时 | 设备专属控制用 **`ioctl()`** |

---

## 4.3 文件描述符 FD

| | |
|--|--|
| 定义 | 非负整数；进程内引用 **已打开文件** |
| 默认 | `0` `STDIN_FILENO` · `1` `STDOUT_FILENO` · `2` `STDERR_FILENO`（`<unistd.h>`） |
| 私有 | fd 是 **进程私有**；两进程同数字 fd 通常指向 **不同** 打开实例 |

→ 内核三层（fd 表 / open file description / inode）见书内 Ch5 · [LKD §3.8](../../04-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-03-process-management/notes/section-3.8-身份PID与资源FD.md)

---

## 4.4 `open()`

```c
#include <fcntl.h>
int open(const char *pathname, int flags, /* mode_t mode */);
```

| | |
|--|--|
| 成功 | 返回 fd |
| 失败 | `-1` |

**flags（位掩码；访问模式三选一）**

| 标志 | 含义 |
|------|------|
| `O_RDONLY` / `O_WRONLY` / `O_RDWR` | 只读 / 只写 / 读写 |
| `O_CREAT` | 不存在则创建；须给 **mode** |
| `O_TRUNC` | 存在则截断为 0 |
| `O_APPEND` | 追加（原子性详 Ch5） |
| `O_NONBLOCK` | 非阻塞打开 |
| `O_NOFOLLOW` | 不跟随符号链接 |

**mode：** 新建权限，受 **umask** 屏蔽：最终权限 ≈ `mode & ~umask`。  
默认 `open` **跟随** 符号链接。

---

## 4.5 `read()`

```c
ssize_t read(int fd, void *buffer, size_t count);
```

| 返回 | 含义 |
|------|------|
| **>0** | 读到的字节数；**可小于 count（短读 short read）** |
| **0** | EOF |
| **-1** | 出错 |

普通文件到尾部常返回 0；管道 / socket / 终端更易短读 → **网络编程必须循环读**。

---

## 4.6 `write()`

```c
ssize_t write(int fd, const void *buffer, size_t count);
```

| 点 | |
|----|--|
| 返回 | 实际写入字节；允许 **部分写 partial write** |
| 落盘 | **成功 ≠ 已持久化**；常先入页缓存（缓冲详后续章）；强制刷盘用 `fsync`/`fdatasync` |

---

## 4.7 `lseek()`

```c
off_t lseek(int fd, off_t offset, int whence);
```

| whence | 基准 |
|--------|------|
| `SEEK_SET` | 文件起始 |
| `SEEK_CUR` | 当前偏移 |
| `SEEK_END` | 文件末尾 |

| 特性 | |
|------|--|
| 只改偏移 | 改的是 **打开文件描述** 内的 offset；**本身不产生 I/O** |
| **文件空洞** | 跳过尾部再 `write`，中间可不占磁盘块；读空洞区得 0 |
| **不可 seek** | 管道、socket、终端 → `-1`，`errno=ESPIPE` |

---

## 4.8 `ioctl()`

```c
int ioctl(int fd, int request, ...);
```

通用流式读写之外：终端、声卡、磁盘等 **设备专属命令**。

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

## 易错清单

1. **fd** 在进程 fd 表；**偏移** 在打开文件描述（open file description）— Ch5 三层结构。  
2. `close(fd)` 释放槽位；进程退出自动关全部 fd。  
3. 管道/socket **不要**假设可 `lseek`。  
4. 短读、部分写是 **正常现象**，必须处理。  
5. `umask` 影响新建权限；`mode` 仅配合 `O_CREAT`。  
6. `write` 成功 ≠ 落盘。

---

## 双线提示

| 路线 | 带走 |
|------|------|
| 嵌入式 | `/dev` 设备也走同一套 open/read/write；专属控制靠 ioctl |
| HFT | 短读/部分写；热路径少 syscall；落盘语义与缓冲要清楚 |

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

## 参考

- 《The Linux Programming Interface》**第 04 章** — File I/O: The Universal I/O Model  
- [OUTLINE](../OUTLINE.md) · 下一内容：书内 Ch5 → [`../chapter-04-file-unbuffered-io/`](../chapter-04-file-unbuffered-io/)
