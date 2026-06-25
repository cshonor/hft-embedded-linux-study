# Ch 14 内核 · Kernel

> **BPF Performance Tools** · Brendan Gregg · **精读 🟡**（内核开发者 🔴）

> 本章定位：**内核本身作为分析目标** — Ch 6–13 借内核观测 **应用**；本章深入 **调度唤醒链、内核锁、Slab/页分配、工作队列**。对 **内核开发者** 极有用；HFT 共置机 **incident 深潜** 时用于「系统卡顿但应用说不清」类问题。  
> **HFT：** 常态 **选读**；`offwaketime` 解阻塞链、`kmem`/`slabratetop` 查内核内存、`mlock` 查内核 mutex。优先 **tracepoint** 而非脆弱 kprobe。与 [05-LKD](../05-Linux-Kernel-Development/) · [06-Gorman](../06-Linux-Virtual-Memory-Manager/) 对照。  
> **上一章：** [chapter-13-应用程序.md](./chapter-13-应用程序.md) · **下一章：** [chapter-15-容器.md](./chapter-15-容器.md)

---

## 1. 本章 vs 前几章

| 视角 | 章 | 问什么 |
|------|-----|--------|
| 借内核看应用 | Ch 6–13 | 策略为何慢？ |
| **看内核自身** | **Ch 14** | 内核在忙什么？谁占 Slab？谁持内核锁？ |

```
应用 offcputime（Ch 13）
        ↓ 仍不清楚阻塞链
wakeuptime / offwaketime（本章）
        ↓ 仍不清楚内核内存
kmem / kpages / slabratetop（本章）
```

---

## 2. 内核基础知识 (Kernel Fundamentals)

### 唤醒 (Wakeups)

线程 **阻塞** 离核（等 I/O、锁、futex…）→ 事件完成 → **另一上下文唤醒** 它。

| 概念 | 说明 |
|------|------|
| **唤醒链** | A 等 B，B 等 C… 形成依赖 |
| **观测价值** | 不只「在等什么」，还有 **「谁把我叫醒」** |

→ 工具：`wakeuptime`、`offwaketime`

→ LKD 调度/等待：[05-LKD Ch 4](../05-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-04-process-scheduling/)

### 内核内存分配

| 分配器 | 作用 |
|--------|------|
| **Slab/Slub** | 固定大小 **内核对象**（`kmalloc`、`kmem_cache`）— 缓存重用 |
| **页分配器** | **按页**（通常 4KiB）连续物理内存 `alloc_pages` |

| 用户态类比 | 内核 |
|------------|------|
| `malloc` | `kmalloc` / Slab |
| `mmap` 大块 | 页分配器 |

→ [06-Gorman](../06-Linux-Virtual-Memory-Manager/) · [Ch 7 用户态内存](./chapter-07-内存.md)

### 内核锁

| 类型 | 特点 |
|------|------|
| **自旋锁** | 忙等，不可睡眠 — 短临界区 |
| **mutex** | 混合：cmpxchg → 乐观自旋 → **睡眠** |
| **rwlock** | 读多写少 |
| **RCU** | 读无锁、延迟回收 — 网络/路径查找常见 |

**BPF：** `mlock`/`mheld` 针对 **mutex**；**自旋锁勿 kretprobe**（书中安全建议）— 用 **CPU profile** 找 spin 热点。

### Tasklets 与 Work Queues

| 下半部 | 说明 |
|--------|------|
| **Tasklet** | 软中断上下文，不可睡眠 |
| **Workqueue** | **内核线程** 执行，可睡眠 — 驱动耗时工作 |

→ 工具：**`workq`** 测 work handler 延迟

→ LKD 中断/下半部：[05-LKD Ch 7–8](../05-Linux-Kernel-Development/)

---

## 3. 传统内核分析工具

### Ftrace

Linux 内置追踪器 — SysPerf 有专章：[chapter-14-ftrace](../02-Systems-Performance-2nd/chapter-14-ftrace/)

| 能力 | 示例 |
|------|------|
| **kprobe + stack** | 函数命中栈 |
| **function graph** | 子调用 **耗时图** |
| 统计 | `echo function > current_tracer` |

```bash
# 示意 — 详见 SysPerf ftrace 笔记
cat /sys/kernel/debug/tracing/available_tracers
```

**BPF vs Ftrace：** BPF 易 **聚合/直方图/过滤**；Ftrace **funcgraph** 对读内核代码流仍极强 — 互补。

### perf sched

```bash
perf sched record -- sleep 10
perf sched latency
```

调度 **延迟、等待、运行时间** — 与 `runqlat`（Ch 6）同族。

### slabtop

```bash
slabtop -o
```

当前 **Slab 缓存占用** — 内核内存压力传统首选。

| | `slabtop` | **`slabratetop`** (BPF) |
|---|-----------|-------------------------|
| 看什么 | **当前总量** | **分配速率** |

---

## 4. 调度与唤醒 (Scheduler & Wakeups)

### `offcputime` — 过滤不可中断休眠

[Ch 6/13](./chapter-13-应用程序.md) 已介绍；本章强调 **状态过滤**：

| 状态 | 含义 |
|------|------|
| **`TASK_UNINTERRUPTIBLE` (D)** | 等 I/O、等锁 — **真实阻塞** |
| 可中断睡眠 | 可能含应用 `sleep()` — **噪音** |

**用法直觉：** Off-CPU 火焰图 **只看 D 状态** → 剔除主动 sleep，聚焦 **I/O/内核锁**。

```bash
# 具体过滤选项见 man offcputime-bpfcc
sudo offcputime-bpfcc -u -p $(pidof myapp) 30
```

### `wakeuptime`

记录 **执行唤醒的线程栈** + **被唤醒线程已阻塞多久**。

```bash
sudo wakeuptime-bpfcc 10
```

**回答：** **谁** 唤醒了沉睡线程 — 唤醒者侧视角。

### `offwaketime` — Off-Wake 火焰图 🔴

结合 **`offcputime` + `wakeuptime`**：

```
上半（倒置）  唤醒者栈  ───┐
                        ├── 交汇 = 唤醒点
下半          被阻塞者栈 ───┘
```

**价值：** 整条 **阻塞 → 唤醒** 链一目了然 — 「神秘系统卡顿」利器。

```bash
sudo offwaketime-bpfcc 30
```

**HFT incident：** 应用 `offcputime` 只有 `futex`/模糊栈 → **`offwaketime`** 追 **谁完成 I/O 并唤醒**。

---

## 5. 内核锁分析

### `mlock` / `mheld`

针对 **内核 mutex**（非 pthread — 见 [Ch 13 `pmlock`](./chapter-13-应用程序.md)）：

| 工具 | 作用 |
|------|------|
| **`mlock`** | mutex **获取延迟** 直方图 + **内核栈** |
| **`mheld`** | **持有者栈** + **持有时长** |

```bash
sudo mlock-bpfcc 10
sudo mheld-bpfcc 10
```

### 自旋锁

| 建议 | 原因 |
|------|------|
| **勿 kretprobe 自旋锁** | 安全/稳定 |
| 用 **`profile`** | 找 **CPU 自旋** 热点 |

**HFT：** 内核侧 spin 在 **网络驱动、极端锁** 场景；用户态热路径应不可见 — 若 profile 内核栈 spin 多 → 驱动/内核版本问题。

---

## 6. 内核内存分析

### `kmem`

追踪 **`kmalloc` 等 Slab 分配** — 按栈统计 **次数、均大小、总字节**。

```bash
sudo kmem-bpfcc 10
```

**场景：** 内核 **泄漏**、某驱动疯狂 alloc。

### `kpages`

追踪 **页级分配** `alloc_pages` — 谁触发 **整页** 分配。

```bash
sudo kpages-bpfcc 10
```

### `slabratetop`

按 **cache 名称** 显示 Slab **分配速率**（实时 top）。

```bash
sudo slabratetop-bpfcc 5
```

**对比 `slabtop`：** 看 **增速** 而非静态快照 — 泄漏/突发 alloc 更敏感。

### `numamove`

**NUMA 页面迁移** 统计 — 自动 NUMA balancing 带来 **意外延迟**。

```bash
sudo numamove-bpfcc
```

**HFT：** 绑核/内存 **local NUMA** 策略下，非零迁移值得查 — 与 [SysPerf Ch 7 内存](../02-Systems-Performance-2nd/chapter-07-memory/) 一致。

---

## 7. 工作队列 — `workq`

追踪 **workqueue** 提交与 **handler 执行延迟** 直方图。

```bash
sudo workq-bpfcc 10
```

**场景：** 驱动/子系统 **下半部** 慢 — 网络、块层常见。

---

## 8. 内核追踪的挑战

| 挑战 | 对策 |
|------|------|
| **kprobe 绑内部函数名** | 内核版本变 → 工具 **碎** |
| 结构体布局变 | 验证器/脚本失败 |
| **优先 tracepoint** | 稳定 ABI — `syscalls:*`、`block:*`、`sched:*` |
| CO-RE / BTF | 新版本工具链 — [Ch 2 BTF](./chapter-02-技术背景.md) |

**原则：** 生产 runbook **优先 BCC 维护工具 + tracepoint**；adhoc kprobe 仅 **短跑验证**。

---

## 9. 工具选型速查

| 症状 | 工具 |
|------|------|
| 阻塞链不清 | **`offwaketime`** |
| 谁唤醒谁 | `wakeuptime` |
| 剔 sleep 噪音的 Off-CPU | `offcputime` + D 状态过滤 |
| 内核 mutex 竞争 | `mlock` / `mheld` |
| 内核 Slab 泄漏/暴涨 | `kmem`、`slabratetop` |
| 大页 alloc 风暴 | `kpages` |
| NUMA 迁移开销 | `numamove` |
| workqueue 慢 | `workq` |
| 读内核代码流 | **Ftrace funcgraph** |

---

## 10. HFT 读者 Takeaway

1. **常态少读** — 除非 **整机卡顿** 且 Ch 6/13/10 无法闭环。
2. **`offwaketime`** — 比单独 `offcputime` 多 **唤醒者** 半条链；共置机 **I/O 完成路径** 排查利器。
3. **内核内存** — `slabratetop` + `kmem` 查 **驱动/内核泄漏**；与用户态 `memleak`（Ch 7）分工。
4. **`mlock` vs `pmlock`** — 内核 mutex vs **pthread**；`syscount` 见 futex 时先 Ch 13。
5. **自旋锁用 profile，勿 kretprobe**。
6. **tracepoint > kprobe** — 内核升级后 runbook 仍可用。
7. 学内核实现：**05-LKD + 06-Gorman** 与本章工具 **对照读**。

---

## 相关章节

- 上一章：[chapter-13-应用程序.md](./chapter-13-应用程序.md)
- 下一章：[chapter-15-容器.md](./chapter-15-容器.md)
- Off-CPU：[chapter-06-CPU.md](./chapter-06-CPU.md) · [chapter-13-应用程序.md](./chapter-13-应用程序.md)
- 用户态内存：[chapter-07-内存.md](./chapter-07-内存.md)
- Ftrace：[chapter-14-ftrace](../02-Systems-Performance-2nd/chapter-14-ftrace/)
- LKD：[05-Linux-Kernel-Development](../05-Linux-Kernel-Development/)
- Gorman：[06-Linux-Virtual-Memory-Manager](../06-Linux-Virtual-Memory-Manager/)
