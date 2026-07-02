## 1. 本章定位

> **《从零自制操作系统》Ch 11 定时器和 ACPI**

---

### 一、要解决什么问题

| Ch 9 用法 | **Ch 11 升级** |
|-----------|----------------|
| **轮询读** Local APIC 计数测耗时 | **定时器中断** — 硬件周期通知 |
| 单次 benchmark | **持续 tick** · **多路逻辑定时器** |
| APIC 频率 **因机器而异** | **ACPI PM** 固定频率 **校准** |

**本章目标：** OS 具备 **毫秒级时间感** 与 **超时消息** — 为多任务 **sleep / 抢占** 做准备。

---

### 二、本章讲什么

| 主题 | 要点 |
|------|------|
| **重构** | `InitializeXXX()` · `std::deque` |
| **APIC Timer** | **Periodic** · 向量 **0x41** |
| **TimerManager** | **`volatile tick_`** · ~**1ms** |
| **priority_queue** | 最近超时优先 · **`kTimerTimeout`** |
| **ACPI PM** | **3.579545 MHz** 参照 |
| **RSDP** | UEFI → ACPI 根指针 · **Checksum** |

---

### 三、在全书中的位置

```
Ch7  中断 + FIFO（鼠标）
Ch9  APIC 计数测量（benchmark）
Ch10 hlt vs 全速循环
    ↓
Ch11 时钟中断 + 多定时器 + ACPI  ← 本章（🔴）
    ↓
Ch13 抢占式多任务
```

→ [Ch9 APIC 测量](../chapter-09-layers/notes/section-4-Local-APIC定时器测量.md)

---

← [Ch 11 导读](../README.md) · 下一节 [2. 重构](./section-2-源码重构.md)
