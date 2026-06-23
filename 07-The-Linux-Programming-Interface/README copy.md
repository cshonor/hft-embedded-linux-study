# TLPI — 《The Linux Programming Interface》学习笔记

个人结构化笔记仓库，按 **第 1～64 章** 独立二级目录管理，每章含 `notes.md` 与 `.gitkeep`，可直接导入 GitHub / NotebookLM。

**姊妹板块**（本 monorepo）：`atomic/`（Rust 并发与 futex 对照）· `async_tokio/` · `rust_network_programming/` · `llvm_insight/`

---

## 书籍简介

*The Linux Programming Interface*（Michael Kerrisk）系统讲解 Linux/UNIX **系统调用与库函数**：文件 I/O、进程、信号、定时器、IPC、POSIX 线程、文件系统、网络套接字、终端与能力机制等。是理解 **Rust `std::os::unix` / `libc` / Tokio 底层** 的经典参考书。

---

## 仓库规范

| 项 | 约定 |
|----|------|
| 目录命名 | 小写 + 连字符：`chapter-NN-topic/` |
| 每章文件 | `.gitkeep`（保空目录进 Git）+ `notes.md`（本章笔记） |
| 示例代码 | 建议放在 `TLPI/examples/chNN/` 或各章目录下 `code/`（按需新建，勿提交二进制） |
| 优先级 | ⭐⭐⭐ 必学 · ⭐⭐ 选学 · ⭐ 延后（见下表） |

**统一笔记模板**：各章 `notes.md` 已预置相同章节结构；可复制 [NOTES_TEMPLATE.md](./NOTES_TEMPLATE.md) 到新文件。

**重新生成骨架**（PowerShell，仓库根目录）：

```powershell
.\TLPI\scripts\bootstrap.ps1
```

---

## 学习进度打卡

在对应章 `notes.md` 顶部勾选「学习状态」，或在下表填 `✅` / `进行中` / `-`。

| 章 | 目录 | 进度 | 优先级 |
|----|------|------|--------|
| 01 | [chapter-01-introduction](./chapter-01-introduction/) | - | ⭐ |
| 02 | [chapter-02-basic-concepts](./chapter-02-basic-concepts/) | - | ⭐⭐ |
| 03 | [chapter-03-file-io](./chapter-03-file-io/) | - | ⭐⭐ |
| 04 | [chapter-04-file-unbuffered-io](./chapter-04-file-unbuffered-io/) | - | ⭐⭐ |
| 05 | [chapter-05-file-attributes](./chapter-05-file-attributes/) | - | ⭐⭐ |
| 06 | [chapter-06-process-environment](./chapter-06-process-environment/) | - | ⭐⭐ |
| 07 | [chapter-07-process-creation](./chapter-07-process-creation/) | - | ⭐⭐⭐ |
| 08 | [chapter-08-process-users-groups](./chapter-08-process-users-groups/) | - | ⭐⭐ |
| 09 | [chapter-09-process-execution](./chapter-09-process-execution/) | - | ⭐⭐⭐ |
| 10 | [chapter-10-signals-basics](./chapter-10-signals-basics/) | - | ⭐⭐⭐ |
| 11 | [chapter-11-signal-handling](./chapter-11-signal-handling/) | - | ⭐⭐⭐ |
| 12 | [chapter-12-signal-advanced](./chapter-12-signal-advanced/) | - | ⭐⭐⭐ |
| 13 | [chapter-13-timers-sleep](./chapter-13-timers-sleep/) | - | ⭐⭐ |
| 14 | [chapter-14-file-locking](./chapter-14-file-locking/) | - | ⭐⭐⭐ |
| 15 | [chapter-15-memory-mapping](./chapter-15-memory-mapping/) | - | ⭐⭐⭐ |
| 16 | [chapter-16-shared-libraries](./chapter-16-shared-libraries/) | - | ⭐⭐ |
| 17 | [chapter-17-interprocess-comm](./chapter-17-interprocess-comm/) | - | ⭐⭐ |
| 18 | [chapter-18-pipes-fifos](./chapter-18-pipes-fifos/) | - | ⭐⭐⭐ |
| 19 | [chapter-19-message-queues](./chapter-19-message-queues/) | - | ⭐⭐ |
| 20 | [chapter-20-semaphores](./chapter-20-semaphores/) | - | ⭐⭐⭐ |
| 21 | [chapter-21-shared-memory](./chapter-21-shared-memory/) | - | ⭐⭐⭐ |
| 22 | [chapter-22-threads-intro](./chapter-22-threads-intro/) | - | ⭐⭐⭐ |
| 23 | [chapter-23-thread-synchronization](./chapter-23-thread-synchronization/) | - | ⭐⭐⭐ |
| 24 | [chapter-24-thread-attributes](./chapter-24-thread-attributes/) | - | ⭐⭐ |
| 25 | [chapter-25-thread-scheduling](./chapter-25-thread-scheduling/) | - | ⭐⭐ |
| 26 | [chapter-26-thread-specific-data](./chapter-26-thread-specific-data/) | - | ⭐⭐ |
| 27 | [chapter-27-process-groups-sessions](./chapter-27-process-groups-sessions/) | - | ⭐⭐ |
| 28 | [chapter-28-daemon-processes](./chapter-28-daemon-processes/) | - | ⭐⭐ |
| 29 | [chapter-29-credentials](./chapter-29-credentials/) | - | ⭐⭐ |
| 30 | [chapter-30-process-resources](./chapter-30-process-resources/) | - | ⭐⭐ |
| 31 | [chapter-31-posix-ipc](./chapter-31-posix-ipc/) | - | ⭐⭐⭐ |
| 32 | [chapter-32-advanced-message-queues](./chapter-32-advanced-message-queues/) | - | ⭐⭐ |
| 33 | [chapter-33-advanced-semaphores](./chapter-33-advanced-semaphores/) | - | ⭐⭐⭐ |
| 34 | [chapter-34-advanced-shared-memory](./chapter-34-advanced-shared-memory/) | - | ⭐⭐ |
| 35 | [chapter-35-file-systems](./chapter-35-file-systems/) | - | ⭐⭐ |
| 36 | [chapter-36-directories-links](./chapter-36-directories-links/) | - | ⭐⭐ |
| 37 | [chapter-37-inodes-files](./chapter-37-inodes-files/) | - | ⭐⭐ |
| 38 | [chapter-38-extended-attributes](./chapter-38-extended-attributes/) | - | ⭐ |
| 39 | [chapter-39-access-control-lists](./chapter-39-access-control-lists/) | - | ⭐ |
| 40 | [chapter-40-monitors](./chapter-40-monitors/) | - | ⭐ |
| 41 | [chapter-41-poll-select](./chapter-41-poll-select/) | - | ⭐⭐⭐ |
| 42 | [chapter-42-epoll](./chapter-42-epoll/) | - | ⭐⭐⭐ |
| 43 | [chapter-43-asynchronous-io](./chapter-43-asynchronous-io/) | - | ⭐⭐⭐ |
| 44 | [chapter-44-memory-allocation](./chapter-44-memory-allocation/) | - | ⭐⭐ |
| 45 | [chapter-45-virtual-memory](./chapter-45-virtual-memory/) | - | ⭐⭐⭐ |
| 46 | [chapter-46-intro-sockets](./chapter-46-intro-sockets/) | - | ⭐⭐⭐ |
| 47 | [chapter-47-socket-api](./chapter-47-socket-api/) | - | ⭐⭐⭐ |
| 48 | [chapter-48-internet-protocols](./chapter-48-internet-protocols/) | - | ⭐⭐⭐ |
| 49 | [chapter-49-domain-names](./chapter-49-domain-names/) | - | ⭐⭐ |
| 50 | [chapter-50-tcp-sockets](./chapter-50-tcp-sockets/) | - | ⭐⭐⭐ |
| 51 | [chapter-51-udp-sockets](./chapter-51-udp-sockets/) | - | ⭐⭐⭐ |
| 52 | [chapter-52-socket-options](./chapter-52-socket-options/) | - | ⭐⭐⭐ |
| 53 | [chapter-53-server-design](./chapter-53-server-design/) | - | ⭐⭐⭐ |
| 54 | [chapter-54-io-multiplexing](./chapter-54-io-multiplexing/) | - | ⭐⭐⭐ |
| 55 | [chapter-55-netlink-sockets](./chapter-55-netlink-sockets/) | - | ⭐ |
| 56 | [chapter-56-terminals](./chapter-56-terminals/) | - | ⭐ |
| 57 | [chapter-57-termios](./chapter-57-termios/) | - | ⭐ |
| 58 | [chapter-58-alternative-io-models](./chapter-58-alternative-io-models/) | - | ⭐⭐ |
| 59 | [chapter-59-psuedo-terminals](./chapter-59-psuedo-terminals/) | - | ⭐ |
| 60 | [chapter-60-advanced-ptys](./chapter-60-advanced-ptys/) | - | ⭐ |
| 61 | [chapter-61-host-info](./chapter-61-host-info/) | - | ⭐⭐ |
| 62 | [chapter-62-program-execution-details](./chapter-62-program-execution-details/) | - | ⭐ |
| 63 | [chapter-63-capabilities](./chapter-63-capabilities/) | - | ⭐⭐⭐ |
| 64 | [chapter-64-final-summary](./chapter-64-final-summary/) | - | ⭐ |

---

## 与 Rust 并发 / 异步 / 网络的对照路线

| TLPI 主题 | 本仓库板块 | 说明 |
|-----------|------------|------|
| 线程、互斥、条件变量（22～26） | `atomic/` | 对照 `std::thread`、`Mutex`、第 8～9 章 futex |
| 信号、定时器（10～13） | `async_tokio/` | 理解信号与异步运行时边界 |
| poll / epoll / AIO（41～43, 54） | `async_tokio/`、`rust_network_programming/` | Tokio/mio 多路复用底层 |
| 套接字 TCP/UDP（46～53） | `rust_network_programming/` | 与网络书 stage 对齐 |
| mmap、虚拟内存（15, 45） | `llvm_insight/` | IR 与内存模型直觉 |

**⭐⭐⭐ 建议优先精读**（打牢系统层再读 Rust 库）：7, 9, 10～12, 14～15, 18, 20～23, 31, 33, 41～43, 45～54, 63。

---

## 示例代码存放指引

```
TLPI/
├── examples/              # 可选：集中存放可编译 C 示例
│   └── ch22-threads/
├── chapter-NN-.../
│   ├── notes.md
│   └── code/              # 可选：本章单文件 demo
└── scripts/
    └── bootstrap.ps1
```

- C 示例：注明 gcc 命令与内核/glibc 版本。  
- Rust 对照：优先 `std::os::unix` / `libc` / `nix`；与 `atomic` 笔记交叉链接。  
- 勿提交编译产物（见 `.gitignore`）。

---

## NotebookLM / AI 精读

1. 上传单章 `notes.md` 或整本目录。  
2. 提示词示例：「按 TLPI 第 N 章列出 syscall 表 + 与 Rust 对照 + 面试题」。  
3. 将输出回写到对应 `notes.md` 的 §3～§7。

---

## 目录树（摘要）

```
TLPI/
├── README.md
├── NOTES_TEMPLATE.md
├── .gitignore
├── scripts/bootstrap.ps1
├── chapter-01-introduction/
│   ├── .gitkeep
│   └── notes.md
├── …
└── chapter-64-final-summary/
    ├── .gitkeep
    └── notes.md
```

共 **64** 个 `chapter-NN-*` 目录，命名与 TLPI 学习路线一致。
