## ⑨ 禁止抢占 · Preemption Disabling

单 CPU 上 **内核抢占** 也会产生 **伪并发**（Ch 9）。

若只保护 **per-CPU 数据** — 对 **当前 CPU 独占**，不必 spinlock：

```c
preempt_disable();
/* 访问 __get_cpu_var(foo) */
preempt_enable();
```

| API | 作用 |
|-----|------|
| **`preempt_disable()`** | 禁止本任务被抢占 |
| **`preempt_enable()`** | 恢复抢占 |

| 注意 | 临界区须 **短** — 同自旋锁精神 |

→ **Ch 8** per-CPU 与 softirq

---
