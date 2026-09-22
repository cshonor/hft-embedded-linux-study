# Ch4 并发类：数据竞争与死锁

> 🔴 精读 · 多线程程序「时对时错」怎么办

**这一章解决什么症状**：结果偶尔出错、偶发死锁、崩溃点漂移不定——多线程程序的「竞态」类问题。并发 bug 是调试里最难的一类：它依赖线程调度时序，往往**偶发、难复现、位置漂移**。

本章三个工具各管一段：gdb 多线程调试（4.1）**停下来看全线程现场**、定位死锁；TSan/Helgrind（4.3）在开发期**全量检测数据竞争**；rr 可逆调试（4.2）**录像回放**复现偶发竞态。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 🟢 4.0′ 第一次数据竞争（零起点：退出码 0 的丢更新 + TSan 报告逐行读） | [00-first-tsan-race.md](notes/00-first-tsan-race.md) |
| 4.1 多线程调试（thread / thread apply all bt / scheduler-locking / 死锁） | `notes/01-thread-debugging.md` |
| 4.2 rr 可逆调试（record / replay / reverse-*） | `notes/02-rr-reversible-debugging.md` |
| 4.3 TSan / Helgrind 数据竞争检测 | `notes/03-threadsanitizer.md` |

---

## 可跑的 demo（`code/`）

| 文件 | 演示什么 | 一句话看点 |
|------|----------|------------|
| `code/c4_1_data_race.c` | 同一件事的四种写法：裸自增 / 互斥锁 / C11 原子 / 每线程私有 | 裸自增静默丢掉 **50%** 的更新，退出码仍是 **0**；TSan 报错那次结果反而是对的 |
| `code/c4_2_deadlock.c` | 确定性 AB-BA 死锁 + 进程内看门狗自证 | 看门狗用 `pthread_mutex_timedlock` 证明「两把锁都被持有 + 没人推进」 |

这两个程序都**不依赖 gdb**，因为它们回答的是同一个问题的两个侧面：4.1 教的
`thread apply all bt` 到底在给你什么信息。先跑 `./c4_2_deadlock abba`，
再看 gdb 那张全线程栈表，就不是天书了。

编译命令、完整实测输出与四个踩坑记录见 [`code/README.md`](code/README.md)。

---

## HFT 关联

- **偶发错单的头号嫌疑是竞态**：下单结果偶发不对，先怀疑多线程共享订单状态没锁好，而不是算法逻辑错（见 1.1 分类学）；
- **死锁定位靠 `thread apply all bt`**：交易系统卡死时，把全线程栈打出来，看谁卡在 `pthread_mutex_lock` 就是死锁签名；
- **rr 复现偶发竞态**：生产偶发的竞态，本地用 rr 录像回放固定时序，把「偶发」变成「可复现」（配合 1.3 最小复现）。
