# Ch 7 · 中断和中断处理程序 · Interrupts and Interrupt Handlers

> **Linux Kernel Development 3rd** · Robert Love · **精读**  
> 本章定位：硬件 **异步打断** → **ISR** → **中断上下文** 规则；**上半部** 与 **下半部** 分工；`request_irq` 与 **关中断** API。HFT **收包延迟、IRQ 绑核、与策略争 cache** 的底层一页。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 中断概念** | IRQ vs 异常 | 异步硬件 · 同步陷阱 |
| **② ISR** | 中断处理程序 | **快** · 中断上下文 |
| **③ 上下半部** | Top / Bottom half | 时限内 vs 可延后 |
| **④ 注册编写** | `request_irq` | `IRQ_HANDLED` · 非可重入 |
| **⑤ 中断上下文** | 与进程上下文对比 | **禁止睡眠** · 中断栈 |
| **⑥ 实现路径** | `do_IRQ` 链 | 硬件 → ISR |
| **⑦ 中断控制** | `local_irq_*` | save/restore |

---

### ① 中断的概念 · Interrupts

硬件用 **电子信号** **异步** 打断 CPU，引起内核注意。

| 概念 | 说明 |
|------|------|
| **IRQ（中断请求线）** | 每个中断对应 **唯一数字** — 内核区分设备 |
| **异步中断** | 外设随时到来 — 网卡、磁盘、键盘… |

#### 中断 vs 异常（同步陷阱）

| 类型 | 来源 | 时机 |
|------|------|------|
| **硬件中断（IRQ）** | 外设 | **异步** — 与当前指令无关 |
| **异常（Exceptions）** | CPU 执行指令 | **同步** — 缺页、非法指令、**syscall 陷入** |

```
异步：  指令流 ──► 指令 ──► [IRQ 插入] ──► ISR ──► 继续
同步：  指令 ──► 触发异常（如 page fault）──► 异常处理
```

→ [Ch 1](./chapter-01-Linux内核简介.md) syscall vs 中断 · [Ch 5](./chapter-05-系统调用.md) 进程上下文

→ 教学对照：[08-1 Day 5 GDT/IDT](../../08-system-low-level-hands-on/08-1-30days-os/notes/day-05-结构体文字显示与GDT-IDT.md) · [Day 7 PIC](../../08-system-low-level-hands-on/08-1-30days-os/notes/day-07-PIC与FIFO.md)

---

### ② 中断处理程序 · Interrupt Handlers (ISR)

| 项 | 说明 |
|----|------|
| **谁写** | **设备驱动** 的一部分 |
| **本质** | 普通 **C 函数** |
| **运行环境** | **中断上下文（interrupt context）** |
| **要求** | **尽可能快** — 尽快恢复被中断代码 |

```
设备 ──IRQ──► CPU ──► ISR（驱动注册）
                         │
                    应答硬件、最小工作
```

**HFT：** 网卡 **每包一次 IRQ**（或合并中断）— ISR/上半部过长 → **P99 尾延迟**、cache 被冲刷。

→ [02 SysPerf §1.5 IRQ 与策略同核](../../02-Systems-Performance-2nd/chapter-01-intro/notes/section-1.5-排障案例与性能挑战.md)

---

### ③ 上半部与下半部 · Top Halves vs Bottom Halves

中断处理常 **既要快又要干很多事** — Linux **拆分**：

| 部分 | 何时跑 | 做什么 |
|------|--------|--------|
| **上半部（Top Half）** | **收到中断立刻** | **时限内必须完成** — ACK、复位硬件、读少量寄存器 |
| **下半部（Bottom Half）** | **稍后**、系统较闲、中断已开 | **非时间关键** — 协议解析、大量拷贝、复杂逻辑 |

```
IRQ ──► 上半部（快、常关该线）
           │
           └── 调度 ──► 下半部（softirq / tasklet / workqueue — Ch 8）
```

| 约束 | 原因 |
|------|------|
| 上半部常 **禁用当前中断线** | 防重入、保硬件状态一致 |
| 重活 **推到下半部** | 缩短关中断窗口 |

→ **Ch 8** 下半部机制详解 · [SysPerf §3.2 上下半部](../../02-Systems-Performance-2nd/chapter-03-operating-systems/notes/section-3.2-内核基础与核心概念.md)

---

### ④ 注册与编写中断处理程序

#### 注册 / 注销

```c
request_irq(irq, handler, flags, name, dev);
free_irq(irq, dev);
```

| 参数/标志 | 含义 |
|-----------|------|
| **irq** | 中断号 |
| **handler** | ISR 函数 |
| **`IRQF_SHARED`** | 多设备 **共享** 同一条 IRQ 线 |
| **`IRQF_DISABLED`**（书中） | 执行时 **禁用本地所有中断**（历史标志；现代驱动少用/已演进） |

#### 返回值 · `irqreturn_t`

| 返回值 | 含义 |
|--------|------|
| **`IRQ_HANDLED`** | 本设备触发且已处理 |
| **`IRQ_NONE`** | 不是本设备（共享 IRQ 时常用） |

#### 可重入性

| 事实 | 推论 |
|------|------|
| 正在跑某线的 ISR 时，该 **IRQ 线全局屏蔽** | 同一条线上 **不会并发** 进同一 handler |
| | ISR **不必写成可重入**（相对该线） |

> 仍须注意：**不同 IRQ 线**、**多 CPU** 与 **共享数据** — 要锁（Ch 9）。

---

### ⑤ 中断上下文 · Interrupt Context

| 对比 | 进程上下文（Ch 5） | 中断上下文 |
|------|-------------------|------------|
| 关联进程 | **`current` 有意义** | **不与具体进程绑定** |
| 睡眠/阻塞 | **可以** | **绝对禁止** — 无「后备进程」可调度 |
| 抢占 | 视内核抢占配置 | 中断本身即抢占 |

#### 栈

| 时代 | 栈布局 |
|------|--------|
| 早期 | ISR **共享** 被中断进程的 **内核栈**（8KB/16KB） |
| 2.6+ 细粒度内核栈（如 4KB） | 每 CPU 独立 **中断栈（interrupt stack）** |

**HFT：** ISR 里 **大数组、递归、`mutex_lock`（可睡眠）** 均为禁忌 — 与 **Ch 2 小栈** 规则叠加。

---

### ⑥ 中断处理机制的实现

硬件 IRQ 到 C 层（书中经典路径，概念仍成立）：

```
硬件 IRQ
    ▼
中断控制器（PIC/APIC/IOAPIC…）
    ▼
CPU 向量 ──► 架构相关汇编入口
    ▼
do_IRQ()          ── 确认、屏蔽该线
    ▼
handle_IRQ_event() ── 遍历该线上注册的 handler
    ▼
各 ISR 执行
    ▼
ret_from_intr()   ── 返回前：调度？下半部？
```

| 阶段 | 要点 |
|------|------|
| **屏蔽 IRQ 线** | 防重入处理同一设备 |
| **返回路径** | 可能 **need_resched**、触发 **下半部**（Ch 4/8） |

> 现代内核 **通用 IRQ 层**（`handle_irq_event` 等）替代部分 `do_IRQ` 细节 — 读书抓 **「向量 → handler 链 → 返回」** 即可。

---

### ⑦ 中断控制 · Interrupt Control

为 **同步数据、避免竞态**，内核提供关/开中断 API。

#### 本地全部中断

| API | 说明 |
|-----|------|
| **`local_irq_disable()`** | 禁止 **本 CPU** 上所有中断 |
| **`local_irq_enable()`** | 开启 |

#### 推荐：保存/恢复

| API | 说明 |
|-----|------|
| **`local_irq_save(flags)`** | 关中断前 **保存** 原状态到 `flags` |
| **`local_irq_restore(flags)`** | **恢复** 之前状态 — 不误开原本就关的中断 |

```c
unsigned long flags;
local_irq_save(flags);
/* 临界区 — 不会被本 CPU 中断打断 */
local_irq_restore(flags);
```

#### 单条 IRQ 线

| API | 作用 |
|-----|------|
| **`disable_irq()` / `enable_irq()`** | **全局** 屏蔽/启用 **特定 IRQ 线** |

#### 自检宏

| 宏 | 作用 |
|----|------|
| **`in_interrupt()`** | 是否在中断（含软中断）相关上下文 |
| **`in_irq()`** | 是否在 **硬 IRQ** 处理中 |

→ **Ch 9–10** 自旋锁 + `local_irq_save` 组合 · [08-1 Day 14 临界区](../../08-system-low-level-hands-on/08-1-30days-os/notes/day-14-高分辨率及键盘输入.md)

---

### Ch 7 小结

| 问题 | 答案 |
|------|------|
| IRQ vs 异常？ | **异步外设** vs **同步 CPU 陷阱** |
| ISR 要求？ | **极快** · **中断上下文** · **不能睡** |
| 为何要下半部？ | 上半部 **时限严** · 重活 **延后**（Ch 8） |
| 怎么注册？ | **`request_irq` / `free_irq`** |
| 共享 IRQ？ | **`IRQF_SHARED`** + `IRQ_NONE`/`HANDLED` |
| 关中断推荐？ | **`local_irq_save` / `restore`** |
| HFT 常做什么？ | **IRQ/NAPI 迁核** · 缩短上半部 · 与用户线程 **NUMA/核隔离** |

---

### 检查单

- [ ] 对比 **中断上下文** 与 **syscall 进程上下文**（能否睡眠）
- [ ] 解释 **上半部 / 下半部** 分工
- [ ] 说出 **`IRQ_HANDLED` vs `IRQ_NONE`**
- [ ] 会用 **`local_irq_save/restore`** 的理由
- [ ] 画 **IRQ → do_IRQ → handler → 下半部** 简图
- [ ] 联系 HFT：**策略核与网卡 IRQ 同核** 的后果

---

## 相关章节

- 上一章：[chapter-06-内核数据结构.md](./chapter-06-内核数据结构.md)
- 下一章：[chapter-08-下半部和推后执行的工作.md](./chapter-08-下半部和推后执行的工作.md)
- 本模块导读：[README.md](./README.md) · [OUTLINE.md](./OUTLINE.md)
