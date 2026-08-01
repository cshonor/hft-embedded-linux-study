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
> glibc 封装 syscall — 对照 [04 LKD Ch5 libc≠syscall](../../04-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-05-system-calls/notes/section-5.1-与内核通信.md)

---

## 4. 标准化：POSIX / SUS / LSB

| 标准 | 作用 |
|------|------|
| **POSIX** | Portable Operating System Interface — 统一 UNIX 系 API，一份代码可在 Linux / FreeBSD / macOS 编译运行 |
| **SUS** | Single UNIX Specification；POSIX 超集。SUSv3≈POSIX.1-2001；SUSv4≈POSIX.1-2008（主流参考） |
| **LSB** | Linux Standard Base — 发行版间兼容（如今影响力渐弱） |

---

## 5. 贯穿全书的最重要结论

| 类型 | 例子 | 可移植性 |
|------|------|----------|
| **POSIX 标准 API** | `fork`、`pipe`、`socket`、`pthread` | Linux / macOS / BSD 通用 |
| **Linux 特有扩展** | `epoll`、`signalfd`、`timerfd`、`io_uring` | **仅 Linux**（macOS/FreeBSD 无） |

### 开发取舍（贴合双线）

| 场景 | 建议 |
|------|------|
| **嵌入式产品、跨平台** | 优先 **POSIX** |
| **HFT / 只跑 Linux** | 大胆用 Linux 扩展（`epoll` / `io_uring` / `mlock`…）；读文档时标清「非标准」 |

---

## 6. 两条路线提示

### 嵌入式 Linux 应用

跨 ARM / 多发行版时 **POSIX 意识**重要。驱动属内核态（LKD 等）；TLPI **只覆盖用户态**。

### HFT

几乎只跑 Linux → 不必强兼容 macOS；可用专属高性能 API。仍要分清哪些 **无法移植**。

---

## 7. 术语清单（极简）

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

## 8. 避坑

1. 本章无实操 — 理解「标准化意义」即可，勿深挖年表。  
2. **macOS 基于 BSD/XNU，不是 Linux**；POSIX 有重合，专属 API / 实现不同。  
3. `epoll` 代码 **不能** 直接当可移植写法搬到 macOS。

---

## 9. 自检

1. **用了 `epoll` 的代码能否直接在 macOS 编译运行？**  
   **不能。** `epoll` 是 Linux 特有；macOS 需用 `kqueue` 等另写。

2. **`pthread` 是 POSIX 还是 Linux 独有？**  
   **POSIX 标准**（Linux/macOS/BSD 均有实现；细节与扩展可不同）。

---

## 10. 背诵卡

| # | 要点 |
|---|------|
| 1 | 商标 UNIX ≠ 习惯「类 UNIX」；Linux 属后者 |
| 2 | 发行版 = Linux 内核 + GNU 用户态 |
| 3 | POSIX 可移植；`epoll`/`io_uring` 等 Linux only |
| 4 | 嵌入式偏 POSIX；HFT 可专吃 Linux 扩展 |

---

## 11. 参考

- 《The Linux Programming Interface》第 01 章 — History and Standards  
- [OUTLINE](../OUTLINE.md) · [模块 README](../README.md)
