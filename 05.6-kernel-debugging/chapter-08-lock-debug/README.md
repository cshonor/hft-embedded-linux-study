# Ch8 Lock Debugging

> Part 3: Diagnostics & Advanced Tools · 🔴 精读

锁调试：LOCKDEP (锁依赖检测器，发现死锁/锁序问题)、KCSAN (并发消毒器，检测数据竞争)、lockdep 的 lock_stat 统计。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 8.1 并发 bug 的类型：死锁 / 活锁 / 数据竞争 | `notes/01-concurrency-bug-types.md` |
| 8.2 LOCKDEP：锁依赖检测器 | `notes/02-lockdep.md` |
| 8.3 用 LOCKDEP 发现潜在死锁 | `notes/03-lockdep-deadlock-detection.md` |
| 8.4 lock_stat：锁竞争统计 | `notes/04-lock-stat.md` |
| 8.5 KCSAN：数据竞争检测器 | `notes/05-kcsan.md` |
| 8.6 在树莓派上启用 LOCKDEP / KCSAN | `notes/06-rpi-lockdep-kcsan.md` |

---

## HFT 关联

精读。并发 bug 是内核中最难调的。LOCKDEP 在开发期能发现锁序问题，KCSAN 能检测无锁变量的数据竞争。HFT 自定义内核模块必须用 LOCKDEP 验证。
