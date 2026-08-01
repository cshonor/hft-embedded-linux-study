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

---
