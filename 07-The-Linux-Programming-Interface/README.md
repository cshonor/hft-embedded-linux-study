# The Linux Programming Interface — Michael Kerrisk（TLPI）

**标准简称：TLPI** · 中文常译：《Linux/UNIX 系统编程手册》

**文件夹 `07`** · [LEARNING-PATH-LOCKED](../LEARNING-PATH-LOCKED.md) · [OUTLINE](./OUTLINE.md) · [CHAPTER-MAP](./CHAPTER-MAP.md) · [READING-LIST](../READING-LIST.md)

> **章号：** 仅 `chapter-01`…`05-file-io-further` 与书对齐；其余目录号是自编顺序，见 [CHAPTER-MAP](./CHAPTER-MAP.md)。

---

## 定位一句话

**用户态 Linux 系统编程圣经** — 聚焦 **应用程序 ↔ 内核** 之间的系统调用与 API。

| | TLPI | LKD / ULK |
|--|------|-----------|
| 站位 | **使用者**：怎么调用内核接口 | **内核开发者**：接口在内核里如何实现 |
| 写什么 | 应用、用户态高性能程序 | 内核子系统、数据结构 |
| 驱动 / 模块 | ❌ **不讲** | LKD 入门；驱动另开专项 |

> **先 TLPI、后 LKD：** 还不会用 `mmap()`，就去啃内核 VM 源码，极易劝退。  
> 锁定路线：**CSAPP → TLPI（+网络）→ 再精读 LKD / Gorman / ULK**（见 [LEARNING-PATH-LOCKED](../LEARNING-PATH-LOCKED.md) Phase3→4）。

---

## 适合两条路线

1. **嵌入式 Linux 应用开发**（求职刚需：进程/文件/IPC/权限）  
2. **HFT 低延迟用户态**（`epoll`、`mmap`、信号、定时器、线程、IPC；**io_uring 需课外补**）

---

## 核心覆盖（日常编码）

| 主题 | 典型 API |
|------|----------|
| 文件 I/O | `open`/`read`/`write`/`mmap`、fd |
| 进程 | `fork`/`exec`/`wait`/`exit` |
| 信号 | `signal` / `sigaction` |
| 线程 | pthread、mutex、条件变量 |
| IPC | 管道、FIFO、共享内存、消息队列、信号量 |
| 定时 / 休眠 | 定时器、`nanosleep` 等 |
| 权限 / 资源 | UID、rlimit |
| Socket 基础 | 进 [UNP](../11-UNP-Vol1/) 前的 Linux 语义 |

深度网络协议 → [UNP](../11-UNP-Vol1/) · [TCP/IP Vol.1](../12-TCP-IP-Illustrated-Vol1/)

---

## 和手头书的阅读顺序

| 书 | 角色 |
|----|------|
| **CSAPP** | 体系、缓存、VM、汇编 — 硬件→进程模型 |
| **TLPI（本目录）** | **用户态系统 API 大全** — Phase3 **优先** |
| **LKD** | 内核子系统实现 — Phase4 |
| **ULK** | 更深内核结构（偏老 2.6）— Phase6 拓展 |

```
CSAPP → TLPI →（动手 / 网络）→ LKD · Gorman →（拓展）ULK
```

**不要**先啃 LKD 再 TLPI。

---

## HFT / 低延迟：优先精读（详见 OUTLINE 🔴）

最短路径（书内章节号，见 [OUTLINE](./OUTLINE.md)）：

```
Ch 2 → 6 → 20–21 → 34–37 → 49 → 29–30 → 58–61 → 63 → 64
```

| 主题 | 为何 |
|------|------|
| **mmap** | 共享内存、少拷贝 |
| 高级 / 非阻塞 I/O | 热路径控制阻塞点 |
| **epoll** | 多路行情接入 |
| 进程 / 线程与调度策略 | 绑核、`SCHED_FIFO`、nice（对照 [04 LKD Ch4](../04-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-04-process-scheduling/)） |
| 时钟 / 高精度定时器 | 节奏与超时 |
| **mlock** | 防换页；低延迟常用 |

---

## 客观短板

1. 书偏老版本 Linux — **没有 io_uring**（现代低延迟核心，需自学补）  
2. **不含** 内核模块、字符设备、设备树、驱动编写 — 只做用户空间；驱动走 Phase5A / 专项资料  

---

## 子目录

| 路径 | 内容 |
|------|------|
| [OUTLINE.md](./OUTLINE.md) | 全书章节 · HFT 🔴/🟡/⚪ 裁剪 |
| `chapter-*/` | 每章 `notes.md` + 按需 `code/`（**目录号可能≠书内章号**，见 [OUTLINE](./OUTLINE.md)） |

---

## 与仓库其他模块

| 模块 | 关系 |
|------|------|
| [01-CSAPP](../01-CSAPP-3rd/) | 进程、信号、I/O 程序员视角地基 |
| [04-LKD](../04-Linux-Kernel-Development/) | 同一批 syscall **在内核里**怎么实现 |
| [05-ULK](../05-Understanding-Linux-Kernel/) | LKD 之后的内核深度 |
| [06-Gorman](../06-Linux-Virtual-Memory-Manager/) | `mmap` 背后的 VM |
| [10-PNP](../10-Practical-Network-Programming/) · [11-UNP](../11-UNP-Vol1/) | 网络实验与 API 纵深 |
| [15-SysPerf](../15-Systems-Performance-2nd/) | 量 epoll / off-CPU |
| [17-HFT](../17-HFT-Low-Latency-Practice/) | 工程落地 |

## 版本

索引默认 **TLPI 第 2 版（2010）** 章节号；与第 1 版大体一致。文件夹名若与书内章号不完全一致，以 [OUTLINE](./OUTLINE.md) 与目录名为准。

---

## 极简背诵

1. 定位：用户态系统调用权威；应用如何与内核交互。  
2. 边界：只覆盖 **用户空间**；不含驱动 / 内核模块。  
3. 价值：嵌入式应用 + HFT 用户态必读。  
4. 次序：**CSAPP 之后、优先于 LKD/ULK**。  
5. 短板：偏老；缺 **io_uring**，需另补。
