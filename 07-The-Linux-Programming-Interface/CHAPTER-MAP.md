# TLPI 书内章号 ↔ 仓库目录

> 对照：[No Starch TOC](https://nostarch.com/tlpi) · [Kerrisk detailed TOC](https://www.michaelkerrisk.com/tlpi/toc-detailed.html)  
> **2026-07-31：** 已按书内章号批量 `git mv`；目录号 = Kerrisk 章号。

---

## 状态

| | |
|--|--|
| **对齐方式** | `chapter-NN-<slug>/` 中的 **NN = 书内章号** |
| **一书多目录** | 同号多 slug（如 Ch15、Ch33、Ch59、Ch62–64、Ch63） |
| **书外内容** | `extras-netlink-sockets/`、`extras-final-summary/` |
| **尚无目录的书内章** | 8, 10, 11, 13, 25, 26, 28, 32, 35, 42, 45（脚手架原缺；读时直接对书） |

---

## 官方目录 → 仓库路径

| # | 官方标题 | 仓库目录 |
|---|----------|----------|
| 1 | History and Standards | `chapter-01-introduction` |
| 2 | Fundamental Concepts | `chapter-02-basic-concepts` |
| 3 | System Programming Concepts | `chapter-03-system-programming-concepts` |
| 4 | File I/O: The Universal I/O Model | `chapter-04-file-io-universal` |
| 5 | File I/O: Further Details | `chapter-05-file-io-further` |
| 6 | Processes | `chapter-06-processes` |
| 7 | Memory Allocation | `chapter-07-memory-allocation` |
| 8 | Users and Groups | *(缺)* |
| 9 | Process Credentials | `chapter-09-process-credentials` |
| 10 | Time | *(缺)* |
| 11 | System Limits and Options | *(缺)* |
| 12 | System and Process Information | `chapter-12-system-process-info` |
| 13 | File I/O Buffering | *(缺)* |
| 14 | File Systems | `chapter-14-file-systems` |
| 15 | File Attributes | `chapter-15-file-attributes` · `chapter-15-inodes-files` |
| 16 | Extended Attributes | `chapter-16-extended-attributes` |
| 17 | Access Control Lists | `chapter-17-access-control-lists` |
| 18 | Directories and Links | `chapter-18-directories-links` |
| 19 | Monitoring File Events | `chapter-19-monitoring-file-events` |
| 20 | Signals: Fundamental Concepts | `chapter-20-signals-fundamentals` |
| 21 | Signals: Signal Handlers | `chapter-21-signal-handlers` |
| 22 | Signals: Advanced Features | `chapter-22-signals-advanced` |
| 23 | Timers and Sleeping | `chapter-23-timers-sleeping` |
| 24 | Process Creation | `chapter-24-process-creation` |
| 25 | Process Termination | *(缺)* |
| 26 | Monitoring Child Processes | *(缺)* |
| 27 | Program Execution | `chapter-27-program-execution` |
| 28 | Process Creation… in More Detail | *(缺)* |
| 29 | Threads: Introduction | `chapter-29-threads-intro` |
| 30 | Threads: Thread Synchronization | `chapter-30-thread-synchronization` |
| 31 | Threads: Thread Safety / TLS | `chapter-31-thread-safety-tsd` |
| 32 | Threads: Thread Cancellation | *(缺)* |
| 33 | Threads: Further Details | `chapter-33-thread-attributes` · `chapter-33-thread-scheduling` |
| 34 | Process Groups, Sessions, Job Control | `chapter-34-process-groups-sessions` |
| 35 | Process Priorities and Scheduling | *(缺)* |
| 36 | Process Resources | `chapter-36-process-resources` |
| 37 | Daemons | `chapter-37-daemons` |
| 38 | Writing Secure Privileged Programs | `chapter-38-secure-privileged` |
| 39 | Capabilities | `chapter-39-capabilities` |
| 40 | Login Accounting | `chapter-40-login-accounting` |
| 41 | Fundamentals of Shared Libraries | `chapter-41-shared-libraries` |
| 42 | Advanced Features of Shared Libraries | *(缺)* |
| 43 | IPC Overview | `chapter-43-ipc-overview` |
| 44 | Pipes and FIFOs | `chapter-44-pipes-fifos` |
| 45 | Introduction to System V IPC | *(缺)* |
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
| 58 | Sockets: TCP/IP Fundamentals | `chapter-58-tcpip-fundamentals` |
| 59 | Sockets: Internet Domains | `chapter-59-internet-domains-dns` · `chapter-59-tcp-sockets` · `chapter-59-udp-sockets` |
| 60 | Sockets: Server Design | `chapter-60-server-design` |
| 61 | Sockets: Advanced Topics | `chapter-61-sockets-advanced` |
| 62 | Terminals | `chapter-62-terminals` · `chapter-62-termios` |
| 63 | Alternative I/O Models | `chapter-63-poll-select` · `chapter-63-epoll` · `chapter-63-asynchronous-io` · `chapter-63-io-multiplexing` · `chapter-63-alternative-io-overview` |
| 64 | Pseudoterminals | `chapter-64-pseudoterminals` · `chapter-64-advanced-ptys` |

### 书外

| 目录 | 说明 |
|------|------|
| `extras-netlink-sockets` | Netlink（原脚手架；非 Kerrisk 独立章） |
| `extras-final-summary` | 总结/延伸阅读 |

---

## HFT 最短路径（目录已对齐）

```
chapter-02 → 03 → 04 → 05 → 20–21 → 23 → 29–30 → 49 → 56–61 → 63-epoll
```
