## ③ 硬件时钟和定时器

架构通常提供 **至少两类** 与时间相关的硬件 — 一类管 **「现在几点」**，一类管 **「多久后叫我一声」**。

#### 两类设备对比

| 设备 | 供电 | 精度/用途 | 内核角色 |
|------|------|-----------|----------|
| **RTC（Real-Time Clock）** | **电池** — 关机仍走 | 秒级日历 | **启动** 时读一次 → 初始化 **墙上时间** |
| **系统定时器 / 本地 APIC Timer** | 主电源 | 可编程 **周期/单次** 中断 | 驱动 **HZ tick**、**clockevents**、**hrtimer 编程** |

| x86 历史组件 | 说明 |
|--------------|------|
| **PIT（8254）** | 老式 **可编程间隔定时器** — 产生周期 tick |
| **HPET** | 高精度 **事件定时器** — 多 comparator |
| **TSC** | **Time Stamp Counter** — CPU 指令周期计数；**测间隔** 极快，**不是** 独立 IRQ 源 |
| **LAPIC Timer** | 每核 **本地** 定时器 — SMP 下 tick 可 **per-CPU** |

```
        ┌────────── RTC（CMOS/NVRAM）──────────┐
        │  2026-07-28 23:00:00  （墙上时间）   │
        └──────────────┬─────────────────────┘
                       │  boot 读一次
                       ▼
                 初始化 xtime（粗）
                       │
  ┌────────────────────┴────────────────────┐
  │  系统定时器 / clockevent 设备            │
  │  编程下一 tick 或下一 hrtimer 事件        │
  └────────────────────┬────────────────────┘
                       │ 周期/单次 IRQ
                       ▼
              timer IRQ handler
              jiffies++ · scheduler · timers
```

#### 启动 vs 运行阶段

| 阶段 | 硬件 | 软件 |
|------|------|------|
| **早期 boot** | RTC → BIOS/EFI 时间 | `mktime` 类逻辑填 **`xtime`** |
| **正常运行** | 系统定时器 **周期性** 或 **动态** 编程 | **`tick_periodic`** / **`hrtimer_interrupt`** |
| **深度 idle** | **下一事件** 才唤醒 | **NO_HZ** — 跳过无意义 tick |

#### TSC 与 HFT（用户态更常碰）

| 用法 | API / 指令 |
|------|------------|
| 内核读间隔 | `ktime_get()`、`local_clock()` |
| 用户态 **RDTSC** / **`rdtscp`** | 极低开销 **时间戳** — 需 **CPU 频率不变** 或 **invariant TSC** |
| 坑 | **频率缩放**、**迁移 CPU** 导致 TSC **不同步** — 实盘绑核 + 检查 **`/proc/cpuinfo`** `constant_tsc` |

**HFT：** 行情 **UTC 对齐** 靠 **PTP/NTP + `CLOCK_REALTIME`**；**延迟测量** 靠 **`CLOCK_MONOTONIC_RAW` 或 TSC**。RTC 只在 **重启后** 给墙上时间 **初值** — 盘中不会每秒读 RTC。

#### 嵌入式 Linux 简图

| SoC | 常见块 |
|-----|--------|
| ARM | **Generic Timer**（arch timer）→ **arch_timer** 驱动 → clockevents |
| 旧板 | ** OMAP/平台 timer** 作 tick 源 |

→ [Ch 11.4 tick 处理](./section-11.4-定时器中断处理程序.md) · [Ch 11.5 xtime](./section-11.5-实际时间-墙上时间.md) · [07 TLPI 时间](../../../../04-linux-userspace-api/)

### 常见陷阱

1. 混淆「时钟源」（clocksource）和「时钟事件设备」（clock_event_device）——前者只读时间，后者可触发中断
2. 以为 HPET 是最佳时钟源——TSC 比 HPET 快 100 倍（~20ns vs ~2us），HPET 是后备
3. 忽略时钟源的稳定性——TSC 在老 CPU 上可能不稳定（频率变化/多核不同步）

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** clocksource 和 clock_event_device 的区别？

<details><summary>答案</summary>

clocksource：只读时钟（单调递增），用于读取当前时间。如 TSC、HPET、ACPI PM Timer。选择最快且稳定的。clock_event_device：可编程定时器，用于设置下一次中断。如 Local APIC Timer、HPET。一个 CPU 上有一个 clocksource（全局共享）和一个 clock_event_device（per-CPU）。

</details>

**Q2.** TSC vs HPET vs ACPI PM Timer 的性能对比？

<details><summary>答案</summary>

TSC（Time Stamp Counter）：~20ns 读取，不变 TSC（invariant TSC）在现代 CPU 上稳定。首选。HPET：~2us 读取，高精度但慢。后备（TSC 不稳定时使用）。ACPI PM Timer：~2us，最后备选。`cat /sys/devices/system/clocksource/clocksource0/current_clocksource` 查看当前使用。HFT 确保 `tsc` 而非 `hpet`。内核启动参数 `clocksource=tsc`。

</details>

**Q3.** HFT 如何确保 TSC 可靠？

<details><summary>答案</summary>

① `cat /proc/cpuinfo | grep constant_tsc`：确认 invariant TSC。② `dmesg | grep -i tsc`：检查内核是否标记 TSC 为 unstable。③ `tsc_reliable` 启动参数：强制标记 TSC 可靠。④ 多核 TSC 同步：现代 CPU 在启动时同步 TSC（`sync_tsc()`），但不同 socket 可能有偏差。⑤ HFT 绑核在同一 socket 上避免跨 socket TSC 偏差。⑥ `RDTSCP` 指令比 `RDTSC` 多一个序列化保证。

</details>

</details>


> ↔ [ULK Ch6 §2 硬件时钟与定时器](../../../../08-linux-kernel-deep/chapter-06-timing/notes/section-2-硬件时钟与定时器.md)
---
