## ⑩ 排序和屏障 · Ordering and Barriers

编译器与 CPU 为性能会 **重排** load/store — 与 **设备寄存器** 或 **SMP 可见性** 冲突时需 **屏障**。

| 屏障 | 作用 |
|------|------|
| **`rmb()`** | **读内存屏障** — 屏障前读不与屏障后读乱序 |
| **`wmb()`** | **写内存屏障** — 写顺序 |
| **`mb()`** | **全屏障** — 读写皆不乱序越过 |
| **`barrier()`** | **编译器屏障** — 防编译器重排，**不约束 CPU** |

```
CPU0:  write A ── wmb() ── write B   → 设备看到 A 先于 B
CPU1:  read B  ── rmb() ── read A   → 配对另一端的发布顺序
```

| SMP | 还需 **`smp_mb()`** 等 — 跨核可见性 |

**HFT：** 用户态 **memory_order_acquire/release**、**发布-消费模式** 与内核屏障 **同一类问题** — 无锁结构错误序 = 极难复现的竞态。

→ [01-CSAPP 并发与内存](../../../../01-CSAPP-3rd/chapter-12-concurrent-programming/)

---
