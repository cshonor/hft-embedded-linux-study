## 选型速查（Ch 9 + Ch 10）

| 场景 | 首选 |
|------|------|
| 单变量计数/标志 | **atomic_t** |
| 短临界区、中断/下半部（不睡） | **spinlock** + `irqsave` |
| 读多写少、写可饿 | **rwlock** |
| 读极多写极少、写不能饿 | **seqlock** |
| 仅当前 CPU 私有数据 | **`preempt_disable`** |
| 进程上下文、长临界区、互斥 | **`mutex`** |
| 计数资源池、需睡眠 | **semaphore** |
| 等另一任务完成事件 | **completion** |
| 跨核/设备可见顺序 | **mb/rmb/wmb** |

---
