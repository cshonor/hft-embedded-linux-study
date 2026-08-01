## 2. 硬件时钟与定时器电路

> 80x86 上内核需与多种时钟/定时器交互

---

### 一、各硬件组件

| 组件 | 作用 |
|------|------|
| **RTC（实时钟）** | 跟踪 **日历时间**（年月日时分秒），掉电常由电池维持 |
| **TSC（时间戳计数器）** | CPU 内 **高精度** 计数器，适合短间隔测量 |
| **PIT（可编程间隔定时器）** | 以固定频率发 **定时器中断**；频率由 **`HZ`** 决定（2.6 常见 1000 Hz = 每 **1 ms** 一 tick） |
| **HPET（高精度事件定时器）** | 较新芯片；多计数器/比较器，精度可达 **10 MHz+** |
| **ACPI PM Timer** | CPU 降频/降压节能时频率 **不变** — 可靠时间源 |

---

### 二、与内核的关系

```
硬件 tick（PIT/HPET/…）
    ↓ 定时器中断
内核 update_times / update_process_times
    ↓
jiffies++、系统时钟、调度 tick
```

→ 中断处理：[Ch 4](../chapter-04-interrupts-and-exceptions/) · SMP APIC 局部 tick [section-3](./section-3-Linux计时架构.md)

---

### 三、HFT 关联

- **TSC** — 用户态/内核 **rdtsc** 测延迟（需 invariant TSC、频率校准）  
- **HZ=1000** vs **tickless/no_hz**（modern）— 2.6 以固定 tick 为主  
- 设备驱动短延迟 → [section-5](./section-5-软件定时器与延迟函数.md) `udelay`

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 计时架构](./section-3-Linux计时架构.md)
