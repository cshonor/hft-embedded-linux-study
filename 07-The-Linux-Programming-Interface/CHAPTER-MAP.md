# TLPI 书内章号 ↔ 仓库目录 对照审计

> 对照来源：[No Starch TLPI TOC](https://nostarch.com/tlpi) · [Kerrisk detailed TOC](https://www.michaelkerrisk.com/tlpi/toc-detailed.html)  
> 审计日期：2026-07-31  
> **结论见文首；全表在下方。**

---

## 总判

| 区间 | 结论 |
|------|------|
| **书内 Ch1–5** | 已对齐（目录号 = 书内章号；笔记标题正确） |
| **书内 Ch6–64** | **大面积错位**：目录是「自编课程序号」，**不是** Kerrisk 章号 |
| **额外问题** | 两个 `chapter-05-*`；OUTLINE 后半用「书内章号」写 HFT 路径，和文件夹对不上 |

**不是**「整书统一偏一位」这种简单错位，而是早期脚手架按另一套顺序建了约 64 个目录，后来只修了开头几章。

---

## 官方目录（书内章号）

| # | 官方标题 |
|---|----------|
| 1 | History and Standards |
| 2 | Fundamental Concepts |
| 3 | System Programming Concepts |
| 4 | File I/O: The Universal I/O Model |
| 5 | File I/O: Further Details |
| 6 | Processes |
| 7 | Memory Allocation |
| 8 | Users and Groups |
| 9 | Process Credentials |
| 10 | Time |
| 11 | System Limits and Options |
| 12 | System and Process Information |
| 13 | File I/O Buffering |
| 14 | File Systems |
| 15 | File Attributes |
| 16 | Extended Attributes |
| 17 | Access Control Lists |
| 18 | Directories and Links |
| 19 | Monitoring File Events |
| 20 | Signals: Fundamental Concepts |
| 21 | Signals: Signal Handlers |
| 22 | Signals: Advanced Features |
| 23 | Timers and Sleeping |
| 24 | Process Creation |
| 25 | Process Termination |
| 26 | Monitoring Child Processes |
| 27 | Program Execution |
| 28 | Process Creation and Program Execution in More Detail |
| 29 | Threads: Introduction |
| 30 | Threads: Thread Synchronization |
| 31 | Threads: Thread Safety and Per-Thread Storage |
| 32 | Threads: Thread Cancellation |
| 33 | Threads: Further Details |
| 34 | Process Groups, Sessions, and Job Control |
| 35 | Process Priorities and Scheduling |
| 36 | Process Resources |
| 37 | Daemons |
| 38 | Writing Secure Privileged Programs |
| 39 | Capabilities |
| 40 | Login Accounting |
| 41 | Fundamentals of Shared Libraries |
| 42 | Advanced Features of Shared Libraries |
| 43 | Interprocess Communication Overview |
| 44 | Pipes and FIFOs |
| 45 | Introduction to System V IPC |
| 46 | System V Message Queues |
| 47 | System V Semaphores |
| 48 | System V Shared Memory |
| 49 | Memory Mappings |
| 50 | Virtual Memory Operations |
| 51 | Introduction to POSIX IPC |
| 52 | POSIX Message Queues |
| 53 | POSIX Semaphores |
| 54 | POSIX Shared Memory |
| 55 | File Locking |
| 56 | Sockets: Introduction |
| 57 | Sockets: UNIX Domain |
| 58 | Sockets: Fundamentals of TCP/IP Networks |
| 59 | Sockets: Internet Domains |
| 60 | Sockets: Server Design |
| 61 | Sockets: Advanced Topics |
| 62 | Terminals |
| 63 | Alternative I/O Models |
| 64 | Pseudoterminals |

---

## 仓库目录 vs 书内（按「笔记标题」猜应属哪一章）

| 仓库目录 | 笔记自称章号/标题 | 应对书内章 | 状态 |
|----------|-------------------|------------|------|
| `chapter-01-introduction` | 01 History and Standards | **1** | ✅ |
| `chapter-02-basic-concepts` | 02 Fundamental Concepts | **2** | ✅ |
| `chapter-03-system-programming-concepts` | 03 System Programming Concepts | **3** | ✅ |
| `chapter-04-file-io-universal` | 04 Universal I/O Model | **4** | ✅ |
| `chapter-05-file-io-further` | 05 Further Details | **5** | ✅ |
| `chapter-05-file-attributes` | 05 File I/O: Metadata | **15** File Attributes | ❌ 重号 + 错号 |
| `chapter-06-process-environment` | 06 Process Environment | **6** Processes（部分） | ⚠️ 标题不全 |
| `chapter-07-process-creation` | 07 Programs and Processes | **6** 或概览；创建是 **24** | ❌ |
| `chapter-08-process-users-groups` | 08 Process Credentials | **9**（8=Users and Groups） | ❌ |
| `chapter-09-process-execution` | 09 Process Execution | **27** Program Execution | ❌ |
| `chapter-10-signals-basics` | 10 Signals Basic | **20** | ❌ |
| `chapter-11-signal-handling` | 11 Signal Handlers | **21** | ❌ |
| `chapter-12-signal-advanced` | 12 Advanced Signals | **22** | ❌ |
| `chapter-13-timers-sleep` | 13 Timers and Sleeping | **23**（书 13=缓冲） | ❌ |
| `chapter-14-file-locking` | 14 File Locking | **55**（书 14=文件系统） | ❌ |
| `chapter-15-memory-mapping` | 15 Memory Mapping | **49**（书 15=文件属性） | ❌ |
| `chapter-16-shared-libraries` | 16 Shared Libraries | **41** | ❌ |
| `chapter-17-interprocess-comm` | 17 IPC Overview | **43** | ❌ |
| `chapter-18-pipes-fifos` | 18 Pipes and FIFOs | **44** | ❌ |
| `chapter-19-message-queues` | 19 System V MQ | **46** | ❌ |
| `chapter-20-semaphores` | 20 System V Semaphores | **47**（书 20=信号） | ❌ |
| `chapter-21-shared-memory` | 21 System V SHM | **48** | ❌ |
| `chapter-22-threads-intro` | 22 POSIX Threads | **29** | ❌ |
| `chapter-23-thread-synchronization` | 23 Thread Sync | **30** | ❌ |
| `chapter-24-thread-attributes` | 24 Thread Attributes | **29–33** 一带 | ❌ |
| `chapter-25-thread-scheduling` | 25 Thread Scheduling | **33** 一带 | ❌ |
| `chapter-26-thread-specific-data` | 26 TSD | **31** | ❌ |
| `chapter-27-process-groups-sessions` | 27 Process Groups… | **34** | ❌ |
| `chapter-28-daemon-processes` | 28 Daemons | **37** | ❌ |
| `chapter-29-credentials` | 29 Credentials 补充 | **9** / **38** | ❌ |
| `chapter-30-process-resources` | 30 Process Resources | **36** | ❌ |
| `chapter-31-posix-ipc` | 31 POSIX IPC Overview | **51** | ❌ |
| `chapter-32-advanced-message-queues` | 32 POSIX MQ | **52** | ❌ |
| `chapter-33-advanced-semaphores` | 33 POSIX Semaphores | **53** | ❌ |
| `chapter-34-advanced-shared-memory` | 34 POSIX SHM | **54** | ❌ |
| `chapter-35-file-systems` | 35 File Systems | **14** | ❌ |
| `chapter-36-directories-links` | 36 Directories and Links | **18** | ❌ |
| `chapter-37-inodes-files` | 37 File Attributes… | **15** | ❌ |
| `chapter-38-extended-attributes` | 38 EA | **16** | ❌ |
| `chapter-39-access-control-lists` | 39 ACL | **17** | ❌ |
| `chapter-40-monitors` | 40 Monitors | **19** Monitoring File Events? | ⚠️ |
| `chapter-41-poll-select` | 41 poll/select | **63** | ❌ |
| `chapter-42-epoll` | 42 epoll | **63** | ❌ |
| `chapter-43-asynchronous-io` | 43 Alternative I/O | **63** | ❌ |
| `chapter-44-memory-allocation` | 44 Memory Allocation | **7** | ❌ |
| `chapter-45-virtual-memory` | 45 Virtual Memory | **50** | ❌ |
| `chapter-46-intro-sockets` | 46 Sockets Intro | **56** | ❌ |
| `chapter-47`–`53` sockets… | 自编号 | **56–61** | ❌ |
| `chapter-54-io-multiplexing` | Alt I/O Advanced | **63** | ❌ |
| `chapter-55-netlink-sockets` | Netlink | **书中无此章** | ➕ 扩展 |
| `chapter-56-terminals` | Terminals | **62** | ❌ |
| `chapter-57-termios` | Termios | **62** | ❌ |
| `chapter-58-alternative-io-models` | Alt I/O Overview | **63** | ❌ |
| `chapter-59`–`60` pty | Pseudoterminals | **64** | ❌ |
| `chapter-61-host-info` | Host info | **12** 一带? | ⚠️ |
| `chapter-62-program-execution-details` | Login Accounting… | **40** | ❌ |
| `chapter-63-capabilities` | Capabilities | **39** | ❌ |
| `chapter-64-final-summary` | Summary | 附录/总结，非书 Ch64 | ⚠️ |

---

## 典型「同号不同书」对照（一眼看出乱）

| 仓库 `chapter-N` | 笔记在讲 | 书内真正的 Ch N |
|------------------|----------|-----------------|
| 13 | Timers | **File I/O Buffering** |
| 14 | File Locking | **File Systems** |
| 15 | mmap | **File Attributes** |
| 20 | System V Semaphores | **Signals** |
| 42 | epoll | **Shared Libraries (Advanced)** |
| 49 | Domain Names | **Memory Mappings** |
| 63 | Capabilities | **Alternative I/O Models** |
| 64 | Final Summary | **Pseudoterminals** |

---

## 建议怎么改

### 方案 A（推荐）：按书内章号重命名目录 + 改正笔记标题

1. 先全部改到临时名（避免重名冲突）  
2. 再改成 `chapter-NN-<官方短名>/`  
3. 笔记第一行统一：`# TLPI 第 NN 章 — <官方标题>`  
4. 一书一章；一书多目录的（如 Ch63 拆成 poll/epoll）保留子目录或合并说明  
5. 书中没有的（Netlink）放到 `extras/` 或 `chapter-xx-extra-netlink`

工作量大（约 50+ 次 `git mv`），但一劳永逸。

### 方案 B（省事）：不改目录号，只维护本映射表

读的时候永远看 **书内章号**；文件夹当「主题标签」。OUTLINE / README 禁止再写「目录号=书内章」。

---

## 当前已正确（勿再动）

- `chapter-01` … `chapter-04-file-io-universal`
- `chapter-05-file-io-further`

待处理优先：去掉重复的 `chapter-05-file-attributes` 编号（应改为 15）。
