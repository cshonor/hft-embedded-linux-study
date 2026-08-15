# Watchdog 机制详解

> 🔴 精读

## 概念详解

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

三层检测:
  1. NMI → 检查 hrtimer 是否运行 → hard lockup
  2. hrtimer → 更新时间戳
  3. watchdog/N 线程 → 检查时间戳 → soft lockup
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

### Watchdog 配置参数

```bash
# 所有 watchdog 参数
ls /proc/sys/kernel/ | grep watchdog
# nmi_watchdog         — hard lockup 检测开关
# soft_watchdog        — soft lockup 检测开关
# watchdog_cpumask     — 检测 CPU 范围
# watchdog_thresh      — 检测阈值

# 设置检测阈值
echo 10 > /proc/sys/kernel/watchdog_thresh  # 默认 10 (soft=20s, hard=10s)

# 启用/禁用
echo 1 > /proc/sys/kernel/nmi_watchdog     # hard lockup 检测
echo 1 > /proc/sys/kernel/soft_watchdog    # soft lockup 检测

# 设置检测 CPU 范围
echo "0-3" > /proc/sys/kernel/watchdog_cpumask  # 只在 CPU 0-3 上检测
```

### watchdog_thresh 详解

```
watchdog_thresh = 10 (默认):
  hrtimer 间隔 = watchdog_thresh / 2 = 5 秒 (实际 ~4 秒)
  soft lockup 阈值 = 2 * watchdog_thresh = 20 秒
  hard lockup 阈值 = watchdog_thresh = 10 秒

watchdog_thresh = 5 (更敏感):
  hrtimer 间隔 = 2.5 秒
  soft lockup 阈值 = 10 秒
  hard lockup 阈值 = 5 秒

watchdog_thresh = 0:
  完全禁用 watchdog
```

### watchdog_cpumask

```bash
# 设置哪些 CPU 参与 watchdog 检测
# 默认: 所有 CPU
cat /proc/sys/kernel/watchdog_cpumask
# 0-63

# HFT 场景: 排除隔离核
# (隔离核上运行 SCHED_FIFO 线程，长时间不调度是正常的)
echo "0-1" > /proc/sys/kernel/watchdog_cpumask
# 只有 CPU 0-1 参与 watchdog 检测
# CPU 2-3 (隔离核) 不检测
```

### watchdog 内核线程

```bash
# 查看 watchdog 线程
ps aux | grep watchdog
# root  10  0  0  ...  [watchdog/0]
# root  11  0  0  ...  [watchdog/1]
# root  12  0  0  ...  [watchdog/2]
# root  13  0  0  ...  [watchdog/3]

# 每个线程绑定到一个 CPU
# 优先级: SCHED_FIFO (最高优先级)
# 作用: 定期检查时间戳是否过期
```

### HFT 关联应用

HFT 隔离核上运行 SCHED_FIFO 线程，长时间不调度是正常行为。应将隔离核从 watchdog 检测范围中排除。

```bash
# HFT 生产环境 watchdog 配置
# /etc/sysctl.d/99-hft.conf

# 保持默认阈值
kernel.watchdog_thresh = 10

# 排除隔离核（CPU 2-3 用于交易）
kernel.watchdog_cpumask = "0-1"

# 确保 hard lockup 检测启用（检测硬件故障）
kernel.nmi_watchdog = 1
```

### Watchdog 检测流程图

```
                    ┌─────────────┐
                    │  NMI 触发    │
                    └──────┬──────┘
                           ↓
                 ┌─────────────────┐
                 │ hrtimer 是否执行?│
                 └──────┬──────┬───┘
                   否   │    是│
                        ↓      ↓
              ┌──────────┐  ┌──────────────┐
              │ HARD     │  │ 更新时间戳    │
              │ LOCKUP!  │  └──────┬───────┘
              └──────────┘         ↓
                          ┌─────────────────┐
                          │ watchdog/N 检查  │
                          │ 时间戳是否过期?   │
                          └──────┬──────┬───┘
                            是   │    否│
                                 ↓      ↓
                           ┌──────────┐  ┌────────┐
                           │ SOFT     │  │ 正常   │
                           │ LOCKUP!  │  └────────┘
                           └──────────┘
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** HFT 隔离核上为什么应该排除 watchdog 检测？

> HFT 交易线程用 SCHED_FIFO 绑定在隔离核上，长时间不调度是设计行为（交易线程持续处理行情）。如果 watchdog 仍在该核检测，会误报 soft lockup。通过 `watchdog_cpumask` 排除隔离核，避免误报。

**Q2:** watchdog 线程在每个 CPU 上如何工作？

> 每 CPU 有一个 watchdog/n 线程（优先级最高 SCHED_FIFO）。它定期被 hrtimer 唤醒，检查时间戳。如果发现时间戳超过 2×thresh 未更新，说明 CPU 被卡住（soft lockup）。hrtimer 本身由时钟中断驱动，如果连时钟中断都不响应则是 hard lockup。

**Q3:** watchdog_thresh = 0 会发生什么？

> 完全禁用 watchdog——既不检测 soft lockup 也不检测 hard lockup。不推荐，因为失去了 CPU 卡死检测能力。HFT 生产环境不应设为 0。

**Q4:** 为什么 watchdog 线程用 SCHED_FIFO 最高优先级？

> 确保 watchdog 线程能被调度运行——如果 CPU 被低优先级线程占用，watchdog 仍能被唤醒检查时间戳。如果 watchdog 都无法运行，说明 CPU 严重卡死（可能是 hard lockup）。

**Q5:** `watchdog_cpumask` 设置为 "0-1" 后，CPU 2-3 上发生 hard lockup 会怎样？

> 不会检测到。`watchdog_cpumask` 同时控制 soft lockup 和 hard lockup 的检测范围。如果隔离核上发生真正的 hard lockup（如硬件故障），watchdog 不会报告。因此隔离核上仍需要硬件看门狗（如外部 WDT）作为最后保障。

</details>

## 交叉引用

- [05.6 ch10 Soft Lockup](../../chapter-10-panic-lockup/notes/02-soft-lockup.md)
- [05.6 ch10 Hard Lockup](../../chapter-10-panic-lockup/notes/03-hard-lockup.md)
- [05.6 ch10 Hangcheck Timer](../../chapter-10-panic-lockup/notes/05-hangcheck-timer.md)
