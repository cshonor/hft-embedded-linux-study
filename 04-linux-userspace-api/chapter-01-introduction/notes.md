# TLPI 第 01 章 — History and Standards

> 对应目录：`chapter-01-introduction/`
> 书内标题：**History and Standards**（历史与标准）

**优先级**：🟡 选读（理清脉络；不必死记年表）  
**路线**：嵌入式 Linux 应用 · HFT 用户态高性能

---

## 章节定位

历史通识章：**无代码**。建立标准概念共识；快速通读即可。  
贯穿全书要分清：**POSIX 标准接口（跨 UNIX）** vs **Linux 独有扩展 API**。

→ 全书定位：[../README.md](../README.md) · 下一章：[../chapter-02-basic-concepts/](../chapter-02-basic-concepts/)

---

## 1. UNIX 的两层定义

| 定义 | 含义 |
|------|------|
| **商标** | 经 The Open Group 认证才合法持有 UNIX 商标；**Linux 未认证** |
| **行业习惯（本书）** | 遵循经典 UNIX 设计哲学、行为兼容 → Linux 属 **类 UNIX** |

---

## 2. UNIX & C 极简时间线

1. **1969** Ken Thompson，Bell 实验室，PDP-7 汇编初代 UNIX  
2. **1973** Ritchie 发明 C；UNIX 内核用 C 重写 → **可移植革命**  
3. **1979** UNIX V7 后分裂两大分支：  
   - **BSD**（伯克利）：TCP/IP 套接字、信号、csh；FreeBSD、macOS XNU 源自此  
   - **System V**：IPC（消息队列、信号量、共享内存）  
4. 分支接口分歧 → 移植成本高 → **催生 POSIX 标准化**

---

## 3. Linux = 内核 + GNU 工具链（必分清）

**GNU**（GNU's Not Unix）是 Stallman 发起的自由软件项目：目标是提供一套**完全自由**、兼容 UNIX 风格的操作系统，重点在**用户态**组件（编译器、编辑器、C 库、shell、常用命令等）。  
Linus 写的是 **Linux 内核**（1991）：管进程、内存、驱动、系统调用等底层；**不含** shell/`ls`/`gcc` 这类用户态程序。  
GNU 自己的内核（Hurd）未成主流；和 Linux 内核一结合，才形成日常说的「Linux 系统」。

| 部件 | 谁 | 内容 |
|------|-----|------|
| **Linux 内核** | Linus 等 | 地基：硬件与资源管理、syscall |
| **GNU 用户态** | Stallman 项目等 | 房子：glibc、bash、gcc、make、coreutils（`ls`/`cp`…） |

**完整发行版 ≈ Linux 内核 + GNU 用户态工具链**（再加包管理、桌面等；其中也有非 GNU 组件，如 systemd）

类比：**内核 = 地基，GNU = 盖在上面的房子**；只有内核几乎没法日常用。

> ❌ 勿混：**Linux ≠ GNU**。内核与用户态是两套独立项目；Linus 没写 GNU，Stallman 也没写 Linux 内核。  
> glibc 封装 syscall — 对照 [04 LKD Ch5 libc≠syscall](../../07-linux-kernel/00_Book_3rd_Notes/chapter-05-system-calls/notes/section-5.1-与内核通信.md)

---

## 4. 标准化：POSIX / SUS / LSB

**一句话（双线共用）：主力标准 POSIX；SUS 当拓展参考；LSB 工程里忘掉。**

| 标准 | 层级 | 作用 | 对本仓库 |
|------|------|------|----------|
| **POSIX**（IEEE 1003.1） | 源码级可移植 | 定义 syscall/文件 I/O/进程/信号/pthread/实时接口、`clock_gettime`、`mmap`… | **主力**；Linux 尽可能兼容（未花钱官方认证，日常按这套语义写） |
| **SUS**（Single UNIX Specification） | POSIX + 额外扩展 | 通过认证才能合法用 **「UNIX」商标** | 了解即可；发行版几乎不花钱认证；写代码很少刻意追 SUS |
| **LSB**（Linux Standard Base） | 曾求跨发行版**二进制**兼容 | 一份编译产物多发行版跑 | **已废**：末版约 2015，停维护；Yocto/主流发行版已撤；嵌入式/HFT **不考虑**。Primer 为何仍写：见 [1.4-lsb-vs-posix](../../11-embedded-boot-build/primer-system-overview/chapter-01-introduction/1.4-lsb-vs-posix.md) |

### 易混考点：Linux ≠ UNIX（商标）

- **UNIX（商标）**：须通过 SUS 等认证。  
- **Linux**：遵循 POSIX 接口语义，但**没有**官方 UNIX 认证 → 习惯称「类 UNIX」。

---

## 5. 贯穿全书：POSIX vs Linux 扩展

| 类型 | 例子 | 可移植性 |
|------|------|----------|
| **POSIX 标准 API** | `fork`、`pipe`、`socket`、`pthread`、`clock_gettime`、`mlock`、`SCHED_FIFO`（实时扩展） | Linux / macOS / BSD 等通用（程度因实现而异） |
| **Linux 特有扩展** | `epoll`、`signalfd`、`timerfd`、`io_uring`、`sched_setaffinity`、大页、`SO_BUSY_POLL` / TPACKET… | **仅 Linux** |

### 嵌入式 Linux（用户态应用）

1. **应用层以 POSIX 为基准**（`open/read/write`、`fork`、`pthread`、信号、定时器、SHM…）→ 易在 x86 PC / ARM 板子间移植。  
2. **驱动 / 内核模块不受 POSIX 约束**——POSIX 只管用户态 API；内核是 Linux 独有（走 LKD）。  
3. SUS：几乎不关心（除非要迁 Solaris/AIX 等真 UNIX）。  
4. LSB：静态链接、裁剪根文件系统场景下跨发行版二进制兼容无价值 → **忽略**。

### HFT（低延迟用户态）

1. **基础仍靠 POSIX**：单调时钟、pthread、实时信号、`mlock`、实时调度等。  
2. **极致延迟会用大量 Linux 私有扩展**（`io_uring`、亲和性、大页、忙轮询 socket…）。  
3. **规则：基础逻辑尽量 POSIX；热路径优化主动用 Linux 专属，放弃可移植。**  
4. SUS / LSB 对固化机房（固定发行版+内核）**无意义**。

### 极简工程守则

1. 学习 / 面试：弄清三者关系，**核心吃透 POSIX**。  
2. 业务：可移植 → 只用 POSIX；HFT/嵌入式底层优化 → 允许 Linux 扩展（文档标清「非标准」）。  
3. SUS：概念即可，不当硬约束。  
4. LSB：历史产物，现代开发不考虑。

---

## 6. 术语清单（极简）

| 术语 | 一句话 |
|------|--------|
| User / Kernel space | 用户态 / 内核态（Ch2 展开） |
| System call | 用户进内核的合法入口 |
| glibc | GNU C 库；封装 syscall + 高层接口 |
| POSIX | 可移植 OS 接口标准 |
| SUS | Single UNIX Specification |
| BSD / System V | 两大经典 UNIX 分支 |
| Linux 内核 vs GNU | 内核 ≠ 用户态工具链 |

---

## 7. 避坑

1. 本章无实操 — 理解「标准化意义」即可，勿深挖年表。  
2. **macOS 基于 BSD/XNU，不是 Linux**；POSIX 有重合，专属 API / 实现不同。  
3. `epoll` 代码 **不能** 直接当可移植写法搬到 macOS。

---

## 8. 自检

1. **用了 `epoll` 的代码能否直接在 macOS 编译运行？**  
   **不能。** `epoll` 是 Linux 特有；macOS 需用 `kqueue` 等另写。

2. **`pthread` 是 POSIX 还是 Linux 独有？**  
   **POSIX 标准**（Linux/macOS/BSD 均有实现；细节与扩展可不同）。

---

## 9. 背诵卡

| # | 要点 |
|---|------|
| 1 | 商标 UNIX ≠ 习惯「类 UNIX」；Linux 属后者（无 SUS 认证） |
| 2 | 发行版 = Linux 内核 + GNU 用户态 |
| 3 | **主力 POSIX**；SUS 了解即可；**LSB 忘掉** |
| 4 | POSIX 可移植；`epoll`/`io_uring` 等 Linux only |
| 5 | 嵌入式用户态偏 POSIX（驱动无 POSIX）；HFT 热路径可专吃 Linux 扩展 |

---

## 10. 参考

- 《The Linux Programming Interface》第 01 章 — History and Standards  
- [OUTLINE](../OUTLINE.md) · [模块 README](../README.md)
