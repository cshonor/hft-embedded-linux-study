# 3.3 Waking Is a Misnomer（唤醒的误称）

> 所属：**Going to Sleep** · [← 章索引](./README.md)

**`wake()` ≠ 立即运行该任务**。

典型语义：

1. 把任务标为 **runnable**
2. 推进执行器就绪队列
3. 调度器在合适时机再次 **`poll`**

因此「唤醒」更像是**登记可运行**，而非 OS 线程意义上的 instant resume。

理解这一点有助于排查「已 wake 但延迟很大」类问题。
