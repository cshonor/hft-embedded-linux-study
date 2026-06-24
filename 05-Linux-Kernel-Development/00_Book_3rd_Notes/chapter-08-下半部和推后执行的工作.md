# Ch 8 · 下半部和推后执行的工作 · Bottom Halves and Deferring Work

> **Linux Kernel Development 3rd** · Robert Love · **精读**  
> 本章定位：把 Ch 7 **上半部** 装不下的活 **推后** — **softirq / tasklet / workqueue** 选型、`ksoftirqd`、与下半部共享数据的锁。HFT **`%soft` 飙高、NAPI、收包路径抖动** 的另一半地图。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 概念** | 为何需要下半部 | 缩短关中断窗口 |
| **② 历史** | BH → task queue → 现代 | **三种机制** |
| **③ softirq** | 静态 · 可同类型多 CPU 并发 | 锁 / per-CPU |
| **④ tasklet** | 基于 softirq | **同类串行** · 驱动首选 |
| **⑤ workqueue** | 工作者线程 | **唯一可睡眠** |
| **⑥ ksoftirqd** | 防饿死用户态 | nice 19 |
| **⑦ 选型** | 决策树 | sleep? → wq |
| **⑧ 锁定** | `local_bh_*` | 与自旋锁配合 |

---

### ① 下半部概念与必要性

**上半部（ISR）** 的硬约束（Ch 7）：

| 限制 | 后果 |
|------|------|
| **异步** | 随时打断别的代码 |
| **不能阻塞/睡眠** | 不能用 mutex、不能等 I/O |
| **常屏蔽中断线** | 关中断越久，系统越迟钝 |

**策略：** 上半部只做 **应答硬件 + 最小拷贝**；其余 **允许延迟** 的工作 → **下半部**，在 **中断全开、相对安全** 的时机跑。

```
IRQ 上半部（极短）
    │ ACK · 摘环 · 入队
    ▼
下半部（可稍长）
    │ 协议处理 · 唤醒 socket · 块 I/O 提交…
    ▼
用户态 / 其他内核路径继续
```

**HFT：** 收包尖刺不只在 **硬 IRQ**，常在 **softirq 网络 RX** — `mpstat` 看 **`%soft`**。

→ [02 SysPerf §3.2 下半部](../../02-Systems-Performance-2nd/chapter-03-operating-systems/notes/section-3.2-内核基础与核心概念.md) · [§1.5 IRQ/softirq 同核](../../02-Systems-Performance-2nd/chapter-01-intro/notes/section-1.5-排障案例与性能挑战.md)

---

### ② 下半部机制的历史与演进

| 机制 | 时代 | 问题 |
|------|------|------|
| **BH（Bottom Half）** | 最早 | **严格串行** · 静态 **32 个** · 扩展极差 |
| **Task Queues** | 过渡 | 仍局限 |
| — | **2.5 废除** BH 与 task queues | — |

**现代 Linux（2.6+）三种下半部：**

| 机制 | 一句话 |
|------|--------|
| **softirq** | 编译期静态 · **性能关键** · 可多 CPU 同类型并发 |
| **tasklet** | 基于 softirq · **动态** · 同类 **不会** 多 CPU 并行 |
| **workqueue** | **内核线程** · **进程上下文** · **可睡眠** |

---

### ③ 软中断 · Softirqs

| 属性 | 说明 |
|------|------|
| 分配 | **编译期静态** 定义 |
| 典型用户 | **网络、块设备** — 最耗时、最性能关键的路径 |
| 并发 | **同一 softirq 类型可在多 CPU 上同时跑** |

#### 开发者义务

| 手段 | 原因 |
|------|------|
| **严密锁** | 共享数据竞态 |
| **per-CPU 变量** | 减少跨核锁争用 |

```
CPU0: NET_RX softirq ──┐
CPU1: NET_RX softirq ──┼──► 可能同时处理不同包 — 共享队列要锁
CPU2: NET_RX softirq ──┘
```

→ [12 Rosen Ch14 NAPI/softirq](../../12-Linux-Kernel-Networking/chapter-14-高级主题.md)

---

### ④ tasklet

| 属性 | 说明 |
|------|------|
| 实现 | 建立在 **`HI_SOFTIRQ`**、**`TASKLET_SOFTIRQ`** 之上 |
| 生命周期 | **动态** 创建、注册 |
| 并发规则 | **相同 tasklet 绝不会在多 CPU 同时执行** |
| | **不同类型** tasklet **可以** 并发 |

| 对比 softirq | tasklet |
|--------------|---------|
| 同类型多 CPU 并行 | **否** — 同类串行 |
| 锁复杂度 | **较低** |
| 适用 | **大多数普通设备驱动** 下半部 **首选** |

```c
/* 概念 */
tasklet_schedule(&my_tasklet);   /* 上半部里调度 */
/* 稍后在某 CPU 上跑 my_tasklet 函数一次 */
```

---

### ⑤ 工作队列 · Work Queues

| 属性 | 说明 |
|------|------|
| 执行者 | **工作者线程（worker threads）** |
| 上下文 | **进程上下文** — 有 `current` |
| 能力 | **唯一允许阻塞/睡眠的下半部** |

| 可做 | 示例 |
|------|------|
| 睡眠 | `mutex_lock`、等信号量 |
| 大块分配 / 块 I/O | 可能触发回收、等磁盘 |

#### 默认队列

| 队列 | 说明 |
|------|------|
| **`events/n`** | 每 CPU 默认 **通用** worker — 驱动不必自建线程 |

```c
schedule_work(&work);           /* 或 queue_work() */
/* 在 events/n 线程里跑 work.func */
```

→ **Ch 6** `kfifo` 中断入队 + workqueue 出队模式

---

### ⑥ ksoftirqd 辅助线程

| 问题 | 软中断风暴时 **立即跑完所有 softirq** → **用户态饿死** |
|------|--------------------------------------------------------|
| 场景 | 网络风暴、softirq **不断重新激活** |

| 解法 | 说明 |
|------|------|
| **`ksoftirqd/n`** | 每 CPU 一个 **低优先级** 内核线程（**nice 19**） |
| 行为 | 负载过重时，**多余 softirq** 交给 ksoftirqd **延后** 处理 |

```
softirq 过多
    ├─► 仍要在中断返回路径处理一部分
    └─► 溢出部分 ──► ksoftirqd/n（别饿死用户态）
```

**HFT：** 行情洪峰时 **`%soft` + ksoftirqd** 与策略线程 **同核** → tail 延迟；结合 **RPS/RSS、IRQ 迁核、DPDK 旁路**。

---

### ⑦ 如何选择下半部机制

| 需求 | 选择 |
|------|------|
| 工作需要 **睡眠 / 阻塞**（信号量、大块内存、块 I/O） | **只能 workqueue** |
| 不睡眠、普通驱动下半部 | **tasklet**（易用、锁简单） |
| 已高度多线程化、**极致扩展**（核心网络栈） | **softirq** |

#### 决策树

```
推后执行的工作
    │
    ├─ 会睡眠/阻塞？ ──是──► workqueue
    │
    └─ 否 ──► 驱动级？ ──是──► tasklet（默认首选）
                  │
                  └─ 核心子系统、要多 CPU 同类型并行？ ──► softirq
```

| 机制 | 上下文 | 睡眠 | 典型 |
|------|--------|------|------|
| softirq | 软中断上下文 | 否 | NET_RX |
| tasklet | 软中断上下文 | 否 | 驱动 defer |
| workqueue | **进程上下文** | **是** | 慢路径 I/O |

---

### ⑧ 锁定与禁用下半部

下半部在 **中断返回后异步** 执行 — 与 ISR、进程上下文 **共享数据** 时必须加锁。

#### `local_bh_disable()` / `local_bh_enable()`

| API | 作用 |
|-----|------|
| **`local_bh_disable()`** | **本 CPU** 禁止 **softirq + tasklet** 处理 |
| **`local_bh_enable()`** | 重新启用 |

| 注意 | 说明 |
|------|------|
| **不包括 workqueue** | worker 是进程上下文，用 **mutex** 等 |
| 常与 **自旋锁** 配合 | 防 **死锁**（持锁时若被下半部抢同锁） |

```c
spin_lock_irqsave(&lock, flags);   /* 常同时关中断 + 关 bh */
/* 临界区 — ISR 与 softirq/tasklet 不会穿插 */
spin_unlock_irqrestore(&lock, flags);
```

→ **Ch 9–10** 内核同步详解

---

### Ch 8 小结

| 问题 | 答案 |
|------|------|
| 为何有下半部？ | 上半部 **快、不能睡、常关中断** |
| 现代三种？ | **softirq · tasklet · workqueue** |
| 谁可睡眠？ | **仅 workqueue** |
| tasklet vs softirq？ | tasklet **同类串行**；softirq **同类可多 CPU 并行** |
| ksoftirqd？ | softirq 过重时 **防饿死用户态** |
| 驱动默认？ | **tasklet** |
| HFT 看什么？ | **`%soft`**、NAPI、IRQ/RPS 与策略 **是否同核** |

---

### 检查单

- [ ] 列出上半部 vs 下半部 **各能/不能** 做什么
- [ ] 解释 **同类 tasklet 不会多 CPU 并行** 的意义
- [ ] 说出 **workqueue 是唯一可阻塞下半部** 的原因（进程上下文）
- [ ] 知道 **`local_bh_disable` 不挡 workqueue**
- [ ] 画 **IRQ → tasklet/softirq → socket 唤醒 → 用户 read** 简图
- [ ] 联系排障：**softirq 不高也可能因同核 cache 冲刷伤 tail**

---

## 相关章节

- 上一章：[chapter-07-中断和中断处理程序.md](./chapter-07-中断和中断处理程序.md)
- 下一章：[chapter-09-内核同步介绍.md](./chapter-09-内核同步介绍.md)
- 本模块导读：[README.md](./README.md) · [OUTLINE.md](./OUTLINE.md)
