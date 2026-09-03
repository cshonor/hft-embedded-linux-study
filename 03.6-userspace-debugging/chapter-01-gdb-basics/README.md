# Ch1 gdb 基础：断点 / 单步 / 栈 / 变量

> 🔴 精读 · 用户态正确性调试的第一块基石

gdb 是用户态调试的核心工具：加载程序、打断点、单步、查看栈帧与变量、反汇编定位。本章覆盖「拿到一个会崩的程序，怎么在 gdb 里把它查明白」的全套基本功。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 1.1 gdb 入门与调试信息（`-g` 编译 / debuginfo / 加载方式） | `notes/01-gdb-intro-build.md` |
| 1.2 断点与观察点（break / 条件断点 / watchpoint） | `notes/02-breakpoints.md` |
| 1.3 栈帧与回溯（backtrace / frame / 调用约定 / 变量查看） | `notes/03-stack-backtrace.md` |

---

## HFT 关联

精读。段错误是交易进程最常见的崩溃形态，`gdb prog core` 加载 core 文件 + `bt` 回溯是**第一现场定位手段**。本章的断点 / 栈帧 / 变量查看，是后续多线程调试（Ch2）与 coredump 分析（Ch3）的基础，缺一不可。
