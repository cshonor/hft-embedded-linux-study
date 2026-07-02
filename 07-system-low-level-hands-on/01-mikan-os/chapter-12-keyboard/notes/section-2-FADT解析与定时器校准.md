## 2. FADT 解析与定时器校准

> **补全 Ch 11 遗留** — 用 **ACPI PM Timer** 校准 APIC，实现 **精准 10ms** 周期。

---

### 一、ACPI 表解析链

```
RSDP（Ch11 已校验）
    ↓
XSDT（扩展系统描述表）— 64 位指针数组
    ↓
FADT（Fixed ACPI Description Table）
    ↓
PM Timer  I/O 端口 / 块地址
```

| 表 | 作用 |
|----|------|
| **XSDT** | 列出 **各 ACPI 表物理地址** |
| **FADT** | **固定硬件信息** — PM Timer、Reset 等 |

**遍历 XSDT：** 匹配表签名 **`FACP`**（FADT 在 ACPI 中的签名）→ 映射/读取 FADT。

→ [Ch11 RSDP](../chapter-11-timer-acpi/notes/section-6-RSDP解析与小结.md)

---

### 二、ACPI PM Timer 校准 APIC

| 步骤 | 说明 |
|------|------|
| 从 **FADT** 读 **PM_TMR** 端口/地址 | 3.579545 MHz |
| 同步读 **APIC Current Count** | 测 ΔA、ΔP |
| 计算 **APIC ticks/sec** | 换算公式（Ch11） |
| 设置 APIC **Initial Count** | 目标 **10ms** 中断一次 |

**10ms：** 比 Ch11 草案 ~1ms 更 **稳** 的演示周期（以书为准）— **tick 与真实时间对齐**。

---

### 三、与键盘章的关系

校准在 **键盘驱动前** 完成 — 后续 **0.5s 光标闪烁** 依赖 **可靠 TimerManager**。

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. USB 键盘](./section-3-USB键盘与键码映射.md)
