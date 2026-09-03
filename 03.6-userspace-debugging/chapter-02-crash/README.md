# Ch2 崩溃类：段错误与栈破坏

> 🔴 精读 · 程序「崩了」怎么办

**这一章解决什么症状**：段错误（SIGSEGV）、SIGABRT、非法指令（SIGILL）、浮点异常（SIGFPE）、栈溢出——程序「崩溃」这一类问题。崩溃的本质是「访问了不该访问的内存 / 执行了不该执行的指令」，而 coredump + gdb 是还原崩溃现场的唯一硬证据。

本章前半（2.1–2.3）学 gdb 基础——断点、单步、栈回溯，是**活体调试**（程序还在跑）；后半（2.4–2.6）学 coredump——崩溃后从内存快照**尸检**。两者互补：能复现就 gdb 活体调，偶发崩溃就抓 core 尸检。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 2.1 gdb 入门与调试信息（-g 编译 / debuginfo / 加载方式） | `notes/01-gdb-intro-build.md` |
| 2.2 断点与观察点（break / 条件断点 / watchpoint） | `notes/02-breakpoints.md` |
| 2.3 栈帧与回溯（backtrace / frame / 调用约定 / 变量查看） | `notes/03-stack-backtrace.md` |
| 2.4 core 文件生成配置（ulimit -c / core_pattern / systemd-coredump） | `notes/04-core-dump-config.md` |
| 2.5 加载 core 回溯（gdb prog core / bt / frame / 现场还原） | `notes/05-load-core-backtrace.md` |
| 2.6 深入内存分析（x 看内存 / 多线程 core / 反汇编 / 与 rr 互补） | `notes/06-analyze-corrupted-memory.md` |

---

## HFT 关联

- **崩溃是交易系统的红线**：一个段错误可能让整个下单链路中断，定位速度决定损失。`bt` + core 回溯是「崩溃后 5 分钟定位到行」的标准动作；
- **偶发崩溃必须开 core**：生产环境崩溃不可复现，唯一证据是 core。上线前务必配好 `ulimit -c` + `core_pattern`（见 2.4），否则崩溃了连现场都没有；
- **符号二进制成对归档**：core 是内存快照，没有配套的调试符号二进制就回溯不到源码行（见 2.5），两者必须一起留档。
