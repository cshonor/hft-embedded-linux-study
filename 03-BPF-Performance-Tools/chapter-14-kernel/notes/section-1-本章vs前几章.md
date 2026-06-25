# 1. 本章 vs 前几章

| 视角 | 章 | 问什么 |
|------|-----|--------|
| 借内核看应用 | Ch 6–13 | 策略为何慢？ |
| **看内核自身** | **Ch 14** | 内核在忙什么？谁占 Slab？谁持内核锁？ |

```
应用 offcputime（Ch 13）
        ↓ 仍不清楚阻塞链
wakeuptime / offwaketime（本章）
        ↓ 仍不清楚内核内存
kmem / kpages / slabratetop（本章）
```

---
