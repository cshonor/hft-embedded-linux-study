# 10.4 Watchdog 机制详解

> 🔴 精读

## 本节要点

### Watchdog 架构

```
每 CPU:
  ┌─────────────────────────────────┐
  │  hrtimer (高精度定时器)          │ ← 每 4 秒触发, 更新 ts
  │  → 更新 watchdog_touch_ts       │
  ├─────────────────────────────────┤
  │  watchdog/N 内核线程            │ ← 检查 ts 是否过期
  │  → 检查 soft lockup             │
  ├─────────────────────────────────┤
  │  NMI handler                    │ ← 检查 hrtimer 是否执行
  │  → 检查 hard lockup             │
  └─────────────────────────────────┘
```

### 时间线

```
正常:
  CPU2: [hrtimer]---4s---[hrtimer]---4s---[hrtimer]---4s---
  watchdog/2: check---OK---check---OK---check---OK---

Soft lockup (CPU 在内核态循环, 但中断仍响应):
  CPU2: [hrtimer]---4s---[hrtimer]---4s---[hrtimer]---4s---
        (中断还在执行, 但不调度)
  watchdog/2: check---OK---check---OK---check---EXPIRED!
  (CPU 上的线程未让出, 20秒后 soft lockup)

Hard lockup (CPU 完全不响应):
  CPU2: [hrtimer]---X---X---X---  (中断不执行!)
  NMI: check---OK---check---EXPIRED!
  (NMI 不可屏蔽, 仍能触发检测)
```

### 配置参数

```bash
# 所有 watchdog 参数
ls /proc/sys/kernel/ | grep watchdog
# nmi_watchdog
# soft_watchdog
# watchdog_cpumask
# watchdog_thresh

# 设置检测 CPU 范围 (排除隔离核)
echo "0-1" > /proc/sys/kernel/watchdog_cpumask  # 只在 CPU 0-1 上检测

# HFT 场景: 隔离核不检测 watchdog
# (因为 RT 线程长时间运行在隔离核上是正常的)
echo "0-1" > /proc/sys/kernel/watchdog_cpumask
```

### HFT 关联

HFT 隔离核上运行 SCHED_FIFO 线程，长时间不调度是正常行为。应将隔离核从 watchdog 检测范围中排除。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** HFT 隔离核上为什么应该排除 watchdog 检测？

> HFT 交易线程用 SCHED_FIFO 绑定在隔离核上，长时间不调度是设计行为（交易线程持续处理行情）。如果 watchdog 仍在该核检测，会误报 soft lockup。通过 `watchdog_cpumask` 排除隔离核，避免误报。


**Q:** watchdog 线程在每个 CPU 上如何工作？

> 每 CPU 有一个 watchdog/n 纠正线程（优先级最高 SCHED_FIFO）。它每 watchdog_thresh 秒被 hrtimer 唤醒，更新时间戳。如果发现时间戳超过 2×thresh 未更新，说明 CPU 被卡住（soft lockup）。hrtimer 本身由时钟中断驱动，如果连时钟中断都不响应则是 hard lockup。

</details>

## 交叉引用

- [05.6 ch10 soft lockup](chapter-10-panic-lockup/notes/section-10-2.md)
