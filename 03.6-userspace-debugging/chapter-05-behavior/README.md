# Ch5 行为类：系统调用与库调用追踪

> 🔴 精读 · 程序「卡住」或「调了不该调的」怎么办

**这一章解决什么症状**：进程卡住不动、打开的文件不对、绑定的端口不对、热路径里有多余的系统调用——「程序到底让内核和库做了什么」这一类行为问题。行为类问题不需要改代码、不需要符号，strace/ltrace 直接看「运行全程的调用流」。

本章四个工具：strace（5.1–5.2）追踪**系统调用**，ltrace（5.3）追踪**库函数调用**，attach（5.4）追踪**已经在跑的进程**。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 5.1 strace 入门（基本用法 / 输出格式 / 参数与 errno 解读） | `notes/01-strace-basics.md` |
| 5.2 strace 实战分析（-c 统计 / -f 子进程 / -p attach / 阻塞与多余 syscall 定位） | `notes/02-strace-practical-analysis.md` |
| 5.3 ltrace 库调用追踪（与 strace 对比 / malloc-free 追踪） | `notes/03-ltrace-library-calls.md` |
| 5.4 attach 运行中进程（gdb -p / ptrace 权限 / fork 跟随） | `notes/04-attach-running-process.md` |

---

## HFT 关联

- **定位卡死**：行情/下单进程卡住，`strace -p` 看它停在 `recv`（等数据）还是 `futex`（等锁），一步区分「网络问题」和「死锁」；
- **发现多余 syscall**：热路径里本可避免的 `gettimeofday`/`read`/系统调用是延迟杀手，`strace -c` 一眼暴露；
- **审计调用链**：追踪 socket 建连、`send`/`recv` 时序，还原下单链路是否按预期走。
