# Ch2 gdb 进阶：多线程调试 / attach / rr 可逆调试

> 🔴 精读 · 交易系统调试的核心战场

交易进程几乎全是多线程 + 常驻运行 + 偶发崩溃，这正是单线程「从头 run 到崩」那套基本功不够用的场景。本章覆盖三个进阶能力：**多线程现场全景**（哪个线程卡在哪把锁）、**attach 不重启抓现场**、**rr 可逆调试**（把偶发崩溃「倒带」重放）。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 2.1 多线程调试（thread / thread apply all bt / scheduler-locking / 死锁） | `notes/01-thread-debugging.md` |
| 2.2 attach 运行中进程（gdb -p / ptrace 权限 / fork 跟随） | `notes/02-attach-running-process.md` |
| 2.3 rr 可逆调试（record / replay / reverse-*） | `notes/03-rr-reversible-debugging.md` |

---

## HFT 关联

精读。三招分别对应交易系统三类最棘手的问题：

- **多线程竞态 → 错单**：`thread apply all bt` 一次性看全线程栈，`scheduler-locking` 精确单步某一线程复现竞态；
- **常驻进程卡死 → 不能重启**：`gdb -p <PID>` attach 现场看它卡在哪个 `recv`/`mutex_lock`；
- **偶发崩溃 → 复现难**：rr 记录运行轨迹，崩溃后 `reverse-continue` 倒回根因。
