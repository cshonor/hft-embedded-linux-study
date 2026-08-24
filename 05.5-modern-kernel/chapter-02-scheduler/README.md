# Ch2 调度器 (CFS → EEVDF)

> 来源: 笨叔《奔跑吧Linux内核》 + LWN.net + Bootlin
> 对标旧书: ULK3 Ch7 / LKD3 Ch4 (O(1)/CFS 已过时)

EEVDF 调度器原理、CFS 历史缺陷、latency-nice 机制。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 2.1 EEVDF 调度器 (LWN) | `notes/01-eevdf-scheduler.md` |
| 2.2 CFS 历史与缺陷 (LWN) | `notes/02-cfs-history.md` |
| 2.3 调度器笔记 (笨叔) | `notes/03-scheduler-ben-shu.md` |
| 2.4 进程调度讲义 (Bootlin) | `notes/04-process-scheduling-bootlin.md` |
| 2.5 任务列表 vs 运行队列 (LKD3 Ch3) | `notes/05-task-list-vs-runqueue.md` |
| 2.6 CFS 运行时机制链 (tick→vruntime→pick→抢占) | `notes/06-cfs-runtime-mechanics.md` |

---

## HFT 关联

EEVDF 的 deadline 机制让交互式任务更快被调度。HFT 交易线程用 SCHED_FIFO 不受 EEVDF 影响，但辅助线程的延迟毛刺会减少。
