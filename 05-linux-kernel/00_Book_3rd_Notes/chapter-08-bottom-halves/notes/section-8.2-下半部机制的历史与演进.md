## ② 下半部机制的历史与演进

Linux 下半部机制经历 **多次重写** — 理解历史可避免在读旧代码/邮件列表时混淆 **「bottom half」** 一词的多层含义。

#### 早期：静态 Bottom Half（BH）

| 属性 | 说明 |
|------|------|
| **数量** | 编译期 **静态 32 个** BH 槽位 |
| **执行** | **全局串行** — 任意时刻 **只有一个** BH 在跑 |
| **问题** | 扩展性极差；一个慢 BH **阻塞所有其他 BH** |
| **状态** | **2.5 内核废除** |

```
早期 BH（已废弃）：
  BH₀ ──► BH₁ ──► BH₂ ──► …   严格串行，无并行
              ↑
         一个卡住 → 全部堵住
```

#### 过渡：Task Queues

| 属性 | 说明 |
|------|------|
| **思路** | 比 BH 灵活的任务队列 |
| **问题** | 仍 **局限** — 不能满足网络等 **高并发** 需求 |
| **状态** | 与旧 BH 一起在 **2.5** 移除 |

#### 现代 Linux（2.6+）三种下半部

| 机制 | 引入思路 | 一句话 |
|------|----------|--------|
| **softirq** | 替代 BH — **静态类型**、**可 per-CPU 并行** | 性能关键路径（网络、块） |
| **tasklet** | 在 softirq 上 **动态** 封装 | 普通驱动 **首选** defer |
| **workqueue** | **内核线程** 执行 | **可睡眠** 的下半部 |

```
演进时间线：
  静态 BH（32 槽 · 全局串行）
        │
        ▼
  Task Queues（过渡）
        │
        ▼  2.5 废除
  ┌─────┴─────┬─────────────┐
  softirq   tasklet    workqueue
  （2.6+ 至今）
```

#### 术语对照（读代码时）

| 说法 | 通常指 |
|------|--------|
| **「bottom half」泛指** | 相对 ISR 的 **所有 defer 机制** |
| **「BH」大写** | 已废弃的 **旧机制** — 现代代码不应出现 |
| **软中断 / softirq** | `HI_SOFTIRQ`、`NET_RX_SOFTIRQ` 等 |
| **tasklet** | `TASKLET_SOFTIRQ` 上跑的 **动态** 小任务 |

#### 为何演进到 softirq + tasklet

| 需求 | 旧 BH 为何不够 |
|------|----------------|
| **NET_RX 多 CPU 并行** | 全局串行 → 单核瓶颈 |
| **动态驱动 defer** | 静态 32 槽不够、无法 per-device |
| **降低锁复杂度** | tasklet **同类串行** — 驱动易写 |

#### 与 Jiffies / 定时器的关系（预告）

| 机制 | 触发方式 |
|------|----------|
| **softirq** | `raise_softirq()` — 中断返回或 `ksoftirqd` |
| **tasklet** | `tasklet_schedule()` — 内部 raise TASKLET softirq |
| **workqueue** | `schedule_work()` — 唤醒 **worker 线程** |
| **定时器** | 到期后常 **softirq（TIMER）** 或 workqueue — Ch 4 调度 |

**HFT：** 现代网络栈 **几乎全在 softirq** — 调优文档里的「下半部」多半指 **NET_RX/TX softirq**，不是 tasklet。旧 BH 只需 **知道已死** 即可。

→ [Ch 8.3](section-8.3-软中断.md) softirq · [Ch 8.4](section-8.4-tasklet.md) tasklet · [Ch 8.5](section-8.5-工作队列.md) workqueue · [12 Rosen Ch14 NAPI](../../13-kernel-networking/chapter-14-advanced-topics/)

### 常见陷阱

1. 把 BH（Bottom Half）当现代机制——BH 在 2.5 已删除，被 softirq/tasklet/workqueue 取代
2. 以为 tasklet 是新机制——tasklet 基于 softirq，且正在被废弃
3. 混淆 workqueue 的旧版（keventd）和现代版（cmwq）——现代 workqueue 是并发可配置的

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 下半部机制的历史演进？哪些已被删除？

<details><summary>答案</summary>

① BH（Bottom Half）：2.0-2.4，全局串行（同一时间只有一个 BH 执行），2.5 删除。② Task Queue：2.0-2.5，复杂且不灵活，2.5 删除。③ softirq：2.3 引入，仍存在。④ tasklet：2.3 引入，基于 softirq，正在被废弃。⑤ workqueue：2.5 引入，2.6.36 改为 cmwq（Concurrency Managed Workqueue），仍存在且推荐。

</details>

**Q2.** tasklet 为什么被废弃？推荐用什么替代？

<details><summary>答案</summary>

tasklet 缺陷：① 同类型全局串行化（不能多 CPU 并发），性能差。② 基于 softirq 不能睡眠。③ API 复杂。替代：需要并发 → workqueue。需要低延迟 → threaded IRQ。需要定时回调 → hrtimer。内核已标记 tasklet 为 deprecated，新代码不应使用。

</details>

**Q3.** 现代 workqueue（cmwq）和旧版有什么区别？

<details><summary>答案</summary>

旧版（2.5-2.6.35）：每 CPU 一个 worker 线程（`events/n`），如果 work 阻塞会卡住该 CPU 所有 work。cmwq（2.6.36+）：动态创建/销毁 worker 线程，blocked worker 时自动 spawn 新的。`alloc_workqueue()` 可配置 `WQ_UNBOUND`（不绑 CPU）、`WQ_HIGHPRI`（高优先级）、`max_active`（最大并发）。

</details>

</details>

---
