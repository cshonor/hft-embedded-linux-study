# Ch2 崩溃类：段错误与栈破坏

> 🔴 精读 · 程序「崩了」怎么办

**这一章解决什么症状**：段错误（SIGSEGV）、SIGABRT、非法指令（SIGILL）、浮点异常（SIGFPE）、栈溢出——程序「崩溃」这一类问题。崩溃的本质是「访问了不该访问的内存 / 执行了不该执行的指令」，而 coredump + gdb 是还原崩溃现场的唯一硬证据。

本章 2.0 先划清 gdb 的**能力边界**（能干什么、与 ASAN 的冲突、看不到内核）；前半（2.1–2.3）学 gdb 基础——断点、单步、栈回溯，是**活体调试**（程序还在跑）；后半（2.4–2.6）学 coredump——崩溃后从内存快照**尸检**。两者互补：能复现就 gdb 活体调，偶发崩溃就抓 core 尸检。末节 2.7 补上**构建配置**这一前置：线上那个 release 二进制到底被改了什么（断言消失、`-O2` 让 UB 显形、发行版加固旗标激活），这解释了「`-O0` 下怎么测都对、一上线就崩」。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 2.0 GDB 总览：能干什么、不能干什么（能力边界 / ASAN 冲突 / 命令速记） | `notes/00-gdb-overview.md` |
| 2.0′ 新手解剖课：第一次段错误，从 139 到根因（🟢 逐行注释，接续 1.0 学前篇） | `notes/00a-first-segfault.md` |
| 2.1 gdb 入门与调试信息（-g 编译 / debuginfo / 加载方式） | `notes/01-gdb-intro-build.md` |
| 2.2 断点与观察点（break / 条件断点 / watchpoint） | `notes/02-breakpoints.md` |
| 2.3 栈帧与回溯（backtrace / frame / 调用约定 / 变量查看） | `notes/03-stack-backtrace.md` |
| 2.4 core 文件生成配置（ulimit -c / core_pattern / systemd-coredump） | `notes/04-core-dump-config.md` |
| 2.5 加载 core 回溯（gdb prog core / bt / frame / 现场还原） | `notes/05-load-core-backtrace.md` |
| 2.6 深入内存分析（x 看内存 / 多线程 core / 反汇编 / 与 rr 互补） | `notes/06-analyze-corrupted-memory.md` |
| 2.7 Debug 构建 vs Release 构建（release 模式 / `NDEBUG` / `-O2` 下的 UB / 帧指针 / 发行版加固） | `notes/07-debug-vs-release-build.md` |

---

## 可跑的 demo（`code/`）

> ⚠️ **gdb 交互式会话与 core 文件的生成/加载在本仓库的验证环境跑不了**（容器没有 gdb、不能改内核参数、没有 systemd），2.4/2.5 里的 gdb 会话与 `core_pattern` 配置是**手册格式示意**。但**「它是怎么死的」和「调用栈长什么样」可以实测**：

| 文件 | 演示什么 |
|------|----------|
| `code/c2_1_crash_types.c` | **六种崩溃，六个信号**：空指针(139)/abort(134)/assert(134)/除零(136)/写穿栈数组(134 + `*** stack smashing detected ***`)/无限递归爆栈(139) |
| `code/c2_2_backtrace.c` | **进程内真实回溯**（`backtrace()`，即 `bt` 的底层机制）：`main → layer_a..d`。★ `-O0` 得 **10 帧**、`-O2` 得 **6 帧**（`layer_a~d` 被内联进 `main`）——2.7「debug vs release」的硬证据 |

编译、运行与期望退出码见 [`code/README.md`](code/README.md)。

---

## HFT 关联

- **崩溃是交易系统的红线**：一个段错误可能让整个下单链路中断，定位速度决定损失。`bt` + core 回溯是「崩溃后 5 分钟定位到行」的标准动作；
- **偶发崩溃必须开 core**：生产环境崩溃不可复现，唯一证据是 core。上线前务必配好 `ulimit -c` + `core_pattern`（见 2.4），否则崩溃了连现场都没有；
- **符号二进制成对归档**：core 是内存快照，没有配套的调试符号二进制就回溯不到源码行（见 2.5），两者必须一起留档；
- **「Debug 过、Release 崩」先查构建配置**：断言被 `-DNDEBUG` 关掉、`-O2` 把 UB 显形、发行版加固旗标在 `-O1+` 才激活——见 2.7，别先怀疑编译器。
