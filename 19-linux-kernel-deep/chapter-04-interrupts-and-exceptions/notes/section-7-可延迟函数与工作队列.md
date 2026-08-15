## 7. 可延迟函数 (Softirqs · Tasklets) 与工作队列

> 中断处理 = **关键部分**（快）+ **非关键部分**（可延迟）

---

### 一、设计原则

| 部分 | 执行时机 | 要求 |
|------|----------|------|
| **关键部分** | 立即，常 **关中断** | 极短、不可阻塞 |
| **非关键部分** | 可 **开中断** 后延迟 | 见下三种机制 |

---

### 二、Softirqs（软中断）

| 特点 | 说明 |
|------|------|
| 上下文 | **中断上下文** — **绝不能阻塞/休眠** |
| 分配 | **静态** 分配 |
| 重入 | **严格可重入**，可多 CPU **并发** 执行 |

---

### 三、Tasklets

| 特点 | 说明 |
|------|------|
| 基础 | 建立在 **softirq** 之上 |
| 序列化 | **同类型 tasklet** 严格串行 — 简化驱动编写 |
| 上下文 | 仍是中断上下文，**不能阻塞** |

网络栈收包后常走 **NET_RX_SOFTIRQ** 等路径。

---

### 四、工作队列 (Work Queues)

若延迟任务需要 **挂起/阻塞**（如等磁盘 I/O）：

- **不能用** softirq/tasklet  
- 交给 **工作队列** → 由 **`worker` 内核线程**（如 `events`）在 **进程上下文** 执行  
- **允许阻塞**

```
硬中断 ISR（快）
    ↓ tasklet / softirq（中断上下文，不阻塞）
    ↓ workqueue（进程上下文，可阻塞）
```

---

### 五、HFT 关联

- 低延迟网络：减少 **中断 → softirq → 协议栈** 层数，或用户态轮询（DPDK）  
- **中断上下文里不能 sleep** — 违反则 kernel BUG

### 常见陷阱

1. 混淆 softirq、tasklet、workqueue——softirq 静态编译、tasklet 基于 softirq、workqueue 基于内核线程可睡眠
2. 以为 tasklet 还在现代内核中被推荐——tasklet 已被标记 deprecated，推荐用 workqueue 或 threaded IRQ 替代
3. 在 softirq 中调用睡眠函数——softirq 上下文不能 `schedule()`/`mutex_lock()`，只能用 `spin_lock()`

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** softirq、tasklet、workqueue 三者的关键区别？

<details><summary>答案</summary>

softirq：编译时静态注册（`DEFINE_PER_CPU`），运行在 softirq 上下文，不可睡眠，性能最高。tasklet：基于 softirq（HI_SOFTIRQ/TASKLET_SOFTIRQ），动态注册，同类型不并发，已 deprecated。workqueue：运行在内核线程（`kworker`），可睡眠/持 mutex/做 I/O，性能最低但最灵活。

</details>

**Q2.** 为什么 tasklet 被 deprecated？推荐用什么替代？

<details><summary>答案</summary>

Tasklet 有设计缺陷：① 同类型 tasklet 全局串行化（不能多 CPU 并发），性能差。② 基于 softirq，不能睡眠。③ API 复杂。推荐替代：需要并发 → workqueue（`alloc_workqueue()` + `queue_work()`）。需要低延迟 → threaded IRQ。需要定时回调 → `hrtimer` + softirq。

</details>

**Q3.** HFT 中 softirq 对延迟有什么影响？怎么排查？

<details><summary>答案</summary>

NIC 收包走 softirq（`NET_RX_SOFTIRQ`），在 `ksoftirqd` 或中断返回时执行。如果 softirq 积压，收包延迟增大。排查：① `/proc/softirqs` 看 `NET_RX` 计数。② `perf top -e irq:softirq:net_rx` 观察执行频率。③ `cat /proc/[pid]/stat` 的 `delayacct_blkio_ticks`。解决：绑 softirq 到非交易核，或用 NAPI/DPDK 轮询。

</details>

</details>

---

← [6. I/O 中断](./section-6-IO中断处理.md) · 下一节 [8. 中断返回](./section-8-中断返回.md)
> ↔ [LKD Ch08 §8.5 工作队列](../../../05-linux-kernel/chapter-08-bottom-halves/notes/section-8.5-工作队列.md)
> ↔ [LKD Ch08 §8.4 tasklet](../../../05-linux-kernel/chapter-08-bottom-halves/notes/section-8.4-tasklet.md)
> ↔ [LKD Ch08 §8.3 软中断](../../../05-linux-kernel/chapter-08-bottom-halves/notes/section-8.3-软中断.md)
> ↔ [LKD Ch07 §7.3 上半部与下半部](../../../05-linux-kernel/chapter-07-interrupts/notes/section-7.3-上半部与下半部.md)
