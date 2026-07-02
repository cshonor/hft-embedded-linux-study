## 6. RSDP 解析与小结

---

### 一、为何需要 RSDP

**PM Timer 端口地址** 在 **ACPI 表** 中 — 解析链：

```
RSDP → RSDT/XSDT → FADT → … → PM Timer 寄存器地址
```

**第一步：** 从 **UEFI 启动时** 找到 **RSDP（Root System Description Pointer）**。

---

### 二、从 EFI_SYSTEM_TABLE 获取

**UEFI 配置表（Configuration Table）** 中搜索 **ACPI GUID** 项 → 指向 **RSDP**。

或 **EFI 已提供** 指针 — 书中从 **`EFI_SYSTEM_TABLE`** 遍历 **ConfigurationTable[]**。

---

### 三、校验 RSDP

| 检查 | 说明 |
|------|------|
| **签名** | **"RSD PTR "**（8 字符，含空格） |
| **Checksum** | 表内字节和 **≡ 0 (mod 256)** — 防损坏 |
| **Revision** | ACPI 1.0 vs 2.0+（RSDT vs XSDT） |

```cpp
if (memcmp(rsdp->Signature, "RSD PTR ", 8) != 0) panic();
if (CalculateChecksum(rsdp, rsdp->Length) != 0) panic();
```

**通过后：** 读 **RsdtAddress / XsdtAddress** — 继续解析 **FADT**（下一章/后续可深）。

→ [Ch1 EfiMain / SystemTable](../chapter-01-hello-world/notes/section-6-C语言过渡与文件格式.md)

---

### 四、本章总结

| 成果 | 说明 |
|------|------|
| **InitializeXXX** | 内核模块化 |
| **APIC Periodic + 0x41** | **时钟中断** |
| **TimerManager + volatile** | **~1ms tick** |
| **priority_queue** | **多定时器** · **kTimerTimeout** |
| **ACPI PM 校准** | APIC **真实频率** |
| **RSDP** | ACPI 解析 **入口** |

```
Ch7  中断架构
    ↓
Ch11 时间 = 中断 + 调度 + 校准  ← 本章
    ↓
Ch13 抢占式多任务（时间片 / sleep）
```

---

### 五、后续索引

| Ch11 主题 | 继续读 |
|----------|--------|
| 键盘 | [chapter-12-keyboard](../chapter-12-keyboard/) ⚪ |
| 多任务 | [chapter-13-multitask1](../chapter-13-multitask1/) 🔴 |
| 01 定时器 | [01 Day 12 timer](../../02-30days-os/day-12-timer1/) |
| ULK 定时 | [04-Understanding-Linux-Kernel chapter-06-timing](../../../04-Understanding-Linux-Kernel/chapter-06-timing/) |

---

← [5. ACPI 校准](./section-5-ACPI-PM定时器校准.md) · [Ch 10](../chapter-10-window/) · [Ch 11 导读](../README.md)
