# Ch10 PREEMPT_RT 实时内核

> 来源: Bootlin Real-Time Training
> 对标旧书: ULK3 无覆盖 / LKD3 简略

可睡眠自旋锁、中断线程化、优先级继承、HFT RT 调优。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 10.1 PREEMPT_RT 核心原理 | `notes/01-preempt-rt-principles.md` |
| 10.2 PREEMPT_RT 调优与 HFT 实践 | `notes/02-preempt-rt-hft-tuning.md` |

---

## HFT 关联

PREEMPT_RT 是 HFT 交易系统的内核底座。SCHED_FIFO + isolcpus + nohz_full + mlockall = 确定性微秒级延迟。
