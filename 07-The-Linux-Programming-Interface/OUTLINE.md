# TLPI · 章节大纲与 HFT 裁剪

> **Michael Kerrisk** · *The Linux Programming Interface*（2nd ed.）·《Linux/UNIX 系统编程手册》  
> **定位：** 用户态系统 API（先会用，再读 LKD 看实现）— 见 [README](./README.md)  
> 标签：🔴 必读 · 🟡 选读 · ⚪ 跳过  
> 锁定读序：Phase3 在 LKD 前 — [LEARNING-PATH-LOCKED](../LEARNING-PATH-LOCKED.md)

## Part I · 系统编程概念

> **目录名与书内章号已对齐**（Ch3–5）。其余章节目录仍可能错位，以「书内章」列为准。

| 书内章 | 主题 | 目录 | 标签 | 要点 |
|--------|------|------|------|------|
| 1 | History and Standards | `chapter-01-introduction` | 🟡 快读 | **POSIX vs Linux 扩展**；内核+GNU |
| 2 | Fundamental Concepts | `chapter-02-basic-concepts` | **🔴** | 用户/内核、syscall、进程、fd、inode |
| 3 | System Programming Concepts | **`chapter-03-system-programming-concepts`** | 🔴 | 库函数 vs syscall、`errno`、可移植宏 |
| 4 | File I/O: Universal I/O Model | **`chapter-04-file-io-universal`** | **🔴** | `open/read/write/close/lseek`；短读/部分写 |
| 5 | File I/O: Further Details | **`chapter-05-file-io-further`** | 🟡 | `dup`/`fcntl`/原子追加/非阻塞 |

⚠️ 勿与 **APUE Ch3**（也是文件 I/O）章号混用；TLPI 通用模型在 **第 4 章**。

## Part II · 文件属性与其后（书内章号继续；后续目录名仍可能错位）

| 书内章 | 主题 | 标签 | HFT 关联 |
|--------|------|------|----------|
| 6+ | 进程环境等（见各 `chapter-*`） | 按 OUTLINE 原裁剪 | — |

## Part III · 进程

| 章 | 主题 | 标签 | HFT 关联 |
|----|------|------|----------|
| 20 | 信号：基本概念 | 🔴 | 热路径信号屏蔽 |
| 21 | 信号：信号处理函数 | 🔴 | `sigaction`、可重入 |
| 22 | 信号：高级话题 | 🟡 | `signalfd`、实时信号 |
| 24–28 | 进程创建、exec、监控 | 🟡 | 守护进程、子进程 |
| 34–37 | 进程组、优先级、调度 | 🔴 | `SCHED_FIFO`、nice、绑核前置 |

## Part IV · 内存

| 章 | 主题 | 标签 | HFT 关联 |
|----|------|------|----------|
| 49 | 内存映射 | 🔴 | `mmap` 共享订单簿、大页 |
| 50 | 虚拟内存操作 | 🟡 | `mlock` 锁内存、防 swap |

## Part V · 线程

| 章 | 主题 | 标签 | HFT 关联 |
|----|------|------|----------|
| 29–30 | 线程介绍、同步 | 🔴 | pthread、mutex 成本 |
| 31–33 | 线程安全、TLS、取消 | 🟡 | 无锁前的 baseline |

## Part VI · IPC（选读为主）

| 章 | 主题 | 标签 | HFT 关联 |
|----|------|------|----------|
| 44–48 | 管道、FIFO | 🟡 | 进程间粗通信 |
| 51–55 | 消息队列、信号量、共享内存 | 🟡 | 多进程行情分发场景 |

## Part VII · 网络 + 高级 I/O

| 章 | 主题 | 标签 | HFT 关联 |
|----|------|------|----------|
| 56–57 | Socket 简介、域名解析 | 🟡 | 进 UNP 前速览 |
| 58–61 | TCP/UDP、socket 选项 | 🔴 | `TCP_NODELAY`、buffer、非阻塞 |
| **63** | **备选 I/O 模型** | **🔴** | **select / poll / epoll** |
| 64 | 高级 I/O：其他话题 | 🟡 | `eventfd`、`timerfd` |
| 65 | 性能监控 | 🟡 | `/proc`、与 SysPerf 衔接 |

## HFT 最短路径（时间紧）

```
书内: Ch2 → Ch4(通用I/O) → Ch5 → … → Ch20–21 → Ch34–37 → Ch49 → Ch29–30 → Ch58–61 → Ch63 → Ch64
目录: chapter-02 → chapter-03-file-io → chapter-04-file-unbuffered-io → …
```

→ 实验代码放各章 `chapter-*/code/` · 网络纵深 → [10-UNP](../11-UNP-Vol1/)
