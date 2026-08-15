# Ch5 中断管理

> 来源: LWN.net + Bootlin
> 对标旧书: ULK3 Ch7-8 (中断处理已大幅变化)

IRQ domain、线程化中断、tasklet 废弃。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 5.1 IRQ Domain (LWN) | `notes/01-irq-domain.md` |
| 5.2 线程化中断 (LWN) | `notes/02-threaded-irq.md` |
| 5.3 中断与同步讲义 (Bootlin) | `notes/03-interrupt-synchronization-bootlin.md` |

---

## HFT 关联

线程化中断让交易线程可以抢占网卡中断处理，减少中断对交易线程的干扰。
