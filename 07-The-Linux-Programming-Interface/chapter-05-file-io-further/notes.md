# TLPI 第 05 章 — File I/O: Further Details

> 对应目录：`chapter-05-file-io-further/`  
> 书名原文：**File I/O: Further Details**  
> **勘误：** 很多人笔误写成第 4 章。  
> - **Ch4** = File I/O: The Universal I/O Model（基础 `open/read/write`）→ [notes](../chapter-04-file-io-universal/notes.md)  
> - **Ch5** = File I/O: Further Details（进阶文件 I/O）← 本章

**优先级**：🟡→🔴（HFT：非阻塞、`pread`/`pwrite`、偏移共享）  
**前置**：[Ch4 Universal I/O](../chapter-04-file-io-universal/notes.md)  
**后置**：[Ch6 Processes](../chapter-06-processes/notes.md) · [Ch13 File I/O Buffering](../chapter-13-file-io-buffering/notes.md)  
**内核对照**：[LKD §3.8 fd / struct file / inode](../../04-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-03-process-management/notes/section-3.8-身份PID与资源FD.md)

---

## 章节目标

揭示 Linux 内核 **三层文件结构**，理解：fd 复制、原子操作、`pread`/`pwrite`、`fcntl`、非阻塞 I/O、大文件支持。

| | |
|--|--|
| **Ch4** | 会用 `open/read/write/lseek`，只认识 fd **数字** |
| **Ch5** | 看透三层结构；解释偏移共享、多进程写覆盖、fd 重定向 |
| **Ch13** | 页缓存、`write` 缓冲、`fsync`（「write 成功 ≠ 已落盘」） |

---

## 5.1 内核三层结构（本章核心 · 必考）

进程内相关的三层，厘清无数文件 I/O 疑难：

| 层 | 谁有 | 存什么 |
|----|------|--------|
| **1. 进程 fd 表** | 每进程独有 | 指向「打开文件描述」的指针 + **fd 标志**（主要是 `FD_CLOEXEC`） |
| **2. 打开文件描述**（open file description） | 内核全局，可被多 fd 共享 | **文件偏移**（`lseek` 改的就是它）、状态标志（`O_APPEND`/`O_NONBLOCK`…）、指向 inode |
| **3. inode** | 磁盘上真实文件 | 大小、权限、磁盘块位置、链接计数等 |

### 关键推论（高频面试点）

| 操作 | 打开文件描述 | 偏移 |
|------|--------------|------|
| 两次独立 `open()` 同一路径 | **两个** | **各自独立** |
| `dup` / `dup2` / `fcntl(F_DUPFD)` | **同一个** | **共享** |
| `fork` 继承 | 通常 **共享** 父打开描述 | **共享**（见 LKD §3.8） |
| `O_APPEND` | write 前内核 **原子** 移到 EOF 再写 | — |

→ `lseek` 改的是 **打开文件描述里的偏移**，不是「fd 数字本身」。

---

## 5.2 `dup` / `dup2` / `dup3`

```c
#include <unistd.h>
int dup(int oldfd);                        /* 最小可用编号，同一打开描述 */
int dup2(int oldfd, int newfd);            /* newfd 已开则先 close；再复制 */
int dup3(int oldfd, int newfd, int flags); /* Linux；可带 O_CLOEXEC */
```

典型用途：重定向 stdin/stdout（shell 管道、`exec` 前重定向）。  
Demo：[`code/dup_share_offset.c`](./code/dup_share_offset.c)

---

## 5.3 `pread` / `pwrite`

```c
ssize_t pread(int fd, void *buf, size_t count, off_t offset);
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);
```

| 特性 | |
|------|--|
| 偏移 | 用参数 `offset`；**不改** 当前文件偏移 |
| 原子性 | 单次调用原子；`lseek`+`read` **组合非原子**（多线程会乱） |
| 场景 | 多线程随机读写、DB 引擎 |
| 限制 | 不可用于管道/socket 等不可 seek 对象 |

Demo：[`code/pread_demo.c`](./code/pread_demo.c)

---

## 5.4 原子操作

### 反例：检查再创建（竞态）

```c
/* ❌ 非原子：两进程可同时「不存在」再各自 create */
if (open("a.txt", O_RDONLY) == -1)
    open("a.txt", O_WRONLY | O_CREAT, mode);
```

```c
/* ✅ 检查+创建一步完成 */
fd = open(path, O_WRONLY | O_CREAT | O_EXCL, mode);
```

`O_EXCL` + `O_CREAT`：已存在则失败。  
Demo：[`code/o_excl_create.c`](./code/o_excl_create.c)

### `O_APPEND` 原子追加

多进程同文件：带 `O_APPEND` 时每次 write 内核先到 EOF 再写。  
无则 `lseek(SEEK_END)+write` 可互相覆盖。

---

## 5.5 `fcntl` — 文件控制

```c
int fcntl(int fd, int cmd, ...);
```

| cmd | 作用 |
|-----|------|
| `F_DUPFD` | 复制 fd（类似 dup） |
| `F_GETFD` / `F_SETFD` | **fd 标志**（进程 fd 表）：主要 `FD_CLOEXEC` |
| `F_GETFL` / `F_SETFL` | **文件状态标志**（打开描述）：可改 `O_APPEND`、`O_NONBLOCK`；**不能**改 `O_RDONLY`/`O_WRONLY`/`O_RDWR` |

### 两组标志勿混

| | 存在哪 | 例子 | 共享？ |
|--|--------|------|--------|
| **fd 标志** | 进程 fd 表项 | `FD_CLOEXEC` | 每个 fd **私有** |
| **文件状态标志** | 打开文件描述 | `O_APPEND`、`O_NONBLOCK` | `dup` 的 fd **共享** |

`FD_CLOEXEC`：`exec` 时自动关该 fd，防泄漏。  
Demo：[`code/fcntl_nonblock.c`](./code/fcntl_nonblock.c)（管道上设 `O_NONBLOCK`）

---

## 5.6 打开标志补充（扩展 Ch4）

| 标志 | 含义 |
|------|------|
| `O_SYNC` | 同步写：数据+元数据落盘才返回 |
| `O_DSYNC` | 主要数据落盘；元数据可延迟（通常比 `O_SYNC` 轻） |
| `O_CLOEXEC` | 打开时设 close-on-exec；避免 `open`+`fcntl` 之间竞态 |
| `O_NOATIME` | 读时不更新 atime，少写盘 |

---

## 5.7 非阻塞 `O_NONBLOCK`

| 对象 | 行为 |
|------|------|
| **普通磁盘文件** | 通常 **无效果**（读盘一般不靠这个变「非阻塞」） |
| 管道 / FIFO / socket / 终端 | 无数据时 `read` 立即 `-1`，`errno=EAGAIN`/`EWOULDBLOCK` |

> `O_NONBLOCK` ≠ 异步 AIO；只是「不睡、立刻返回」的轮询式语义。

---

## 5.8 大文件 LFS

历史：32 位 `off_t` 约 2GB 上限。

| 做法 | |
|------|--|
| 推荐 | 编译 `-D_FILE_OFFSET_BITS=64`，`off_t` 变 64 位 |
| 备选 | `off64_t` / `open64` / `lseek64` |

---

## 5.9 `/dev/fd`（Linux）

`/dev/fd/n` 访问约等于操作 fd `n`；shell 重定向常用。

---

## 易错清单

1. 偏移在 **打开文件描述**；`dup` 共享，独立 `open` 独立。  
2. `FD_CLOEXEC` = fd 标志；`O_APPEND`/`O_NONBLOCK` = 文件状态标志。  
3. `pread` ≠ `lseek`+`read`（原子性 + 不改全局偏移）。  
4. `O_EXCL` 须配 `O_CREAT`。  
5. `fcntl(F_SETFL)` **改不了** 读写模式。  
6. `O_NONBLOCK` 对普通磁盘文件通常无效。

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

## 章节链路

```
Ch4 会用 fd 数字
  → Ch5 三层结构解释「怪现象」
  → Ch6 进程环境
  → Ch13 缓冲：write 成功 ≠ 落盘（fsync）
```

---

## 双线提示

| 路线 | |
|------|--|
| 嵌入式 | `dup2` 重定向；`O_CLOEXEC` 防 exec 泄漏；设备 fd 的 `fcntl` |
| HFT | `pread`/`pwrite` 并发；少 `lseek` 竞态；非阻塞多用于 socket/管道 |

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

## 参考

- 《The Linux Programming Interface》**第 05 章** — File I/O: Further Details  
- [OUTLINE](../OUTLINE.md) · [Ch4](../chapter-04-file-io-universal/notes.md) · [Ch13](../chapter-13-file-io-buffering/notes.md) · [LKD §3.8](../../04-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-03-process-management/notes/section-3.8-身份PID与资源FD.md)
