# TLPI 书内章号 ↔ 仓库目录

> 对照：[No Starch TOC](https://nostarch.com/tlpi) · [Kerrisk detailed TOC](https://www.michaelkerrisk.com/tlpi/toc-detailed.html)  
> **规则：一书一目录** — `chapter-NN-*` 中 NN = 书内章号，**1–64 各恰好一个文件夹**。

---

## 官方目录 → 仓库路径（完整）

| # | 官方标题 | 仓库目录 |
|---|----------|----------|
| 1 | History and Standards | `chapter-01-introduction` |
| 2 | Fundamental Concepts | `chapter-02-basic-concepts` |
| 3 | System Programming Concepts | `chapter-03-system-programming-concepts` |
| 4 | File I/O: The Universal I/O Model | `chapter-04-file-io-universal` |
| 5 | File I/O: Further Details | `chapter-05-file-io-further` |
| 6 | Processes | `chapter-06-processes` |
| 7 | Memory Allocation | `chapter-07-memory-allocation` |
| 8 | Users and Groups | `chapter-08-users-and-groups` |
| 9 | Process Credentials | `chapter-09-process-credentials` |
| 10 | Time | `chapter-10-time` |
| 11 | System Limits and Options | `chapter-11-system-limits` |
| 12 | System and Process Information | `chapter-12-system-process-info` |
| 13 | File I/O Buffering | `chapter-13-file-io-buffering` |
| 14 | File Systems | `chapter-14-file-systems` |
| 15 | File Attributes | `chapter-15-file-attributes` |
| 16 | Extended Attributes | `chapter-16-extended-attributes` |
| 17 | Access Control Lists | `chapter-17-access-control-lists` |
| 18 | Directories and Links | `chapter-18-directories-links` |
| 19 | Monitoring File Events | `chapter-19-monitoring-file-events` |
| 20 | Signals: Fundamental Concepts | `chapter-20-signals-fundamentals` |
| 21 | Signals: Signal Handlers | `chapter-21-signal-handlers` |
| 22 | Signals: Advanced Features | `chapter-22-signals-advanced` |
| 23 | Timers and Sleeping | `chapter-23-timers-sleeping` |
| 24 | Process Creation | `chapter-24-process-creation` |
| 25 | Process Termination | `chapter-25-process-termination` |
| 26 | Monitoring Child Processes | `chapter-26-monitoring-child-processes` |
| 27 | Program Execution | `chapter-27-program-execution` |
| 28 | Process Creation and Program Execution in More Detail | `chapter-28-process-creation-exec-detail` |
| 29 | Threads: Introduction | `chapter-29-threads-intro` |
| 30 | Threads: Thread Synchronization | `chapter-30-thread-synchronization` |
| 31 | Threads: Thread Safety and Per-Thread Storage | `chapter-31-thread-safety-tsd` |
| 32 | Threads: Thread Cancellation | `chapter-32-thread-cancellation` |
| 33 | Threads: Further Details | `chapter-33-threads-further` |
| 34 | Process Groups, Sessions, and Job Control | `chapter-34-process-groups-sessions` |
| 35 | Process Priorities and Scheduling | `chapter-35-process-priorities-scheduling` |
| 36 | Process Resources | `chapter-36-process-resources` |
| 37 | Daemons | `chapter-37-daemons` |
| 38 | Writing Secure Privileged Programs | `chapter-38-secure-privileged` |
| 39 | Capabilities | `chapter-39-capabilities` |
| 40 | Login Accounting | `chapter-40-login-accounting` |
| 41 | Fundamentals of Shared Libraries | `chapter-41-shared-libraries` |
| 42 | Advanced Features of Shared Libraries | `chapter-42-shared-libraries-advanced` |
| 43 | Interprocess Communication Overview | `chapter-43-ipc-overview` |
| 44 | Pipes and FIFOs | `chapter-44-pipes-fifos` |
| 45 | Introduction to System V IPC | `chapter-45-sysv-ipc-intro` |
| 46 | System V Message Queues | `chapter-46-sysv-message-queues` |
| 47 | System V Semaphores | `chapter-47-sysv-semaphores` |
| 48 | System V Shared Memory | `chapter-48-sysv-shared-memory` |
| 49 | Memory Mappings | `chapter-49-memory-mappings` |
| 50 | Virtual Memory Operations | `chapter-50-virtual-memory` |
| 51 | Introduction to POSIX IPC | `chapter-51-posix-ipc-intro` |
| 52 | POSIX Message Queues | `chapter-52-posix-message-queues` |
| 53 | POSIX Semaphores | `chapter-53-posix-semaphores` |
| 54 | POSIX Shared Memory | `chapter-54-posix-shared-memory` |
| 55 | File Locking | `chapter-55-file-locking` |
| 56 | Sockets: Introduction | `chapter-56-sockets-intro` |
| 57 | Sockets: UNIX Domain | `chapter-57-sockets-unix-domain` |
| 58 | Sockets: Fundamentals of TCP/IP Networks | `chapter-58-tcpip-fundamentals` |
| 59 | Sockets: Internet Domains | `chapter-59-internet-domains` |
| 60 | Sockets: Server Design | `chapter-60-server-design` |
| 61 | Sockets: Advanced Topics | `chapter-61-sockets-advanced` |
| 62 | Terminals | `chapter-62-terminals` |
| 63 | Alternative I/O Models | `chapter-63-alternative-io` |
| 64 | Pseudoterminals | `chapter-64-pseudoterminals` |

### 书外（非 Kerrisk 独立章）

| 目录 | 说明 |
|------|------|
| `extras-netlink-sockets` | Netlink |
| `extras-final-summary` | 总结 / 延伸阅读 |

---

## HFT 最短路径

```
chapter-02 → 03 → 04 → 05 → 20–21 → 23 → 29–30 → 35 → 49 → 56–61 → 63
```
