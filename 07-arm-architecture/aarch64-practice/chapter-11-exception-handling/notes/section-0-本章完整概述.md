# Ch11 完整总结 · 异常处理

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · **精读** · 飞控底层必备  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md) · [Pi5 适配](../../PI5-ADAPT.md)

---

## 本章定位

异常（Exception）是 ARMv8 的核心机制：SVC 系统调用、IRQ 中断、页错误、未定义指令都走同一套框架。本章建立 VBAR→向量表→保存现场→处理→恢复的完整链路。

---

## 11.1 异常类型

| 类型 | 触发原因 | 同步/异步 |
|------|----------|-----------|
| **SVC** | `SVC #0` 指令（系统调用） | 同步 |
| **IRQ** | 外部硬件中断 | 异步 |
| **FIQ** | 快速中断（高优先级 IRQ） | 异步 |
| **SError** | System Error（异步外部错误） | 异步 |
| **同步异常** | 页错误、未定义指令、对齐错误、SP 对齐错误 | 同步 |

> **同步** = 当前指令触发的，PC 可确定（ELR 精确）  
> **异步** = 与当前指令无关，中断随时到来

---

## 11.2 异常等级切换

```
EL0 (用户态) --SVC--> EL1 (内核态)
EL0/EL1 --IRQ--> EL1 (内核态中断处理)
EL1 --HVC--> EL2 (Hypervisor)
EL2/EL1 --SMC--> EL3 (Secure Monitor)
```

- 异常总是**升到更高或同等级**（不能异常降级）
- 返回用 `ERET`：硬件恢复 ELR→PC、SPSR→PSTATE，切回原等级

---

## 11.3 异常向量表（VBAR）⭐

每个 EL 有自己的向量基址寄存器：`VBAR_EL1`、`VBAR_EL2`、`VBAR_EL3`。

向量表有 **16 个表项**，每项 **128 字节**：

```
VBAR_EL1 + 0x000:  当前EL SP0, 同步异常
VBAR_EL1 + 0x080:  当前EL SP0, IRQ
VBAR_EL1 + 0x100:  当前EL SP0, FIQ
VBAR_EL1 + 0x180:  当前EL SP0, SError

VBAR_EL1 + 0x200:  当前EL SPx, 同步异常
VBAR_EL1 + 0x280:  当前EL SPx, IRQ
VBAR_EL1 + 0x300:  当前EL SPx, FIQ
VBAR_EL1 + 0x380:  当前EL SPx, SError

VBAR_EL1 + 0x400:  低EL→当前EL AArch64, 同步
VBAR_EL1 + 0x480:  低EL→当前EL AArch64, IRQ
VBAR_EL1 + 0x500:  低EL→当前EL AArch64, FIQ
VBAR_EL1 + 0x580:  低EL→当前EL AArch64, SError

VBAR_EL1 + 0x600:  低EL→当前EL AArch32, 同步
VBAR_EL1 + 0x680:  低EL→当前EL AArch32, IRQ
VBAR_EL1 + 0x700:  低EL→当前EL AArch32, FIQ
VBAR_EL1 + 0x780:  低EL→当前EL AArch32, SError
```

> 16 项 = 4 种异常类型 × 4 种来源场景。每项 128B 可放 32 条指令（足够跳转到处理函数）。

### 设置向量表

```asm
// 在 EL1 设置向量表
adrp x0, vector_table
add  x0, x0, #:lo12:vector_table
msr   VBAR_EL1, x0
isb

// 向量表定义
.align 11       // 2048 字节对齐（16 × 128 = 2048）
vector_table:
    .align 7    // 128 字节对齐
    b   sync_handler_sp0
    .align 7
    b   irq_handler_sp0
    // ... 共 16 项
```

---

## 11.4 硬件保存 + 软件保存 ⭐

### 硬件自动保存

| 保存到 | 原值 | 说明 |
|--------|------|------|
| `ELR_ELx` | PC | 异常发生时的指令地址（同步异常=触发指令；异步=下一条指令） |
| `SPSR_ELx` | PSTATE | 异常发生时的处理器状态（NZCV、DAIF、EL 等） |
| SP 切换 | — | 切到目标 EL 的 SP（SP_ELx） |

### 软件必须保存

**X0-X30 通用寄存器硬件不自动保存！** 必须在异常处理入口手动压栈。

```asm
irq_handler:
    sub sp, sp, #272          // 保存 31 个寄存器（31×8=248）+ 对齐
    stp x0,  x1,  [sp, #0]
    stp x2,  x3,  [sp, #16]
    stp x4,  x5,  [sp, #32]
    // ... 保存 x0-x29
    stp x29, x30, [sp, #232]
    // 保存 x30(LR)——异常处理中可能调用函数

    // 处理中断
    bl  do_irq

    // 恢复
    ldp x0,  x1,  [sp, #0]
    // ... 恢复 x0-x29
    ldp x29, x30, [sp, #232]
    add sp, sp, #272

    eret                     // 返回：ELR→PC, SPSR→PSTATE
```

> **ERET** 是异常返回的核心：原子恢复 PC 和 PSTATE，切回原 EL。

---

## 11.5 异常综合征（ESR）

同步异常发生时，`ESR_ELx` 包含异常原因：

```asm
mrs x0, ESR_EL1         // 读异常综合征
lsr x1, x0, #26         // EC (Exception Class) 在 bit[31:26]
```

| EC 值 | 含义 |
|-------|------|
| 0x15 | SVC 系统调用 |
| 0x20 | 在 EL0 的指令中止（页错误取指） |
| 0x24 | 在 EL0 的数据中止（页错误访存） |
| 0x25 | 在 EL1 的数据中止 |
| 0x22 | 在 EL0 的对齐错误 |

`FAR_ELx` 保存触发数据异常的虚拟地址。

---

## 11.6 EL2 → EL1 实验

BenOS 启动时可能在 EL2/EL3，需要降到 EL1：

```asm
check_el:
    mrs x0, CurrentEL
    lsr x0, x0, #2
    cmp x0, #3
    b.eq from_el3
    cmp x0, #2
    b.eq from_el2
    b   in_el1

from_el3:
    // 配置 SCR_EL3 允许 HVC 和非安全
    // eret 到 EL2
from_el2:
    // 配置 HCR_EL2
    // 设置 EL1 的 SCTLR/HCR
    // eret 到 EL1
in_el1:
    // 设置 VBAR_EL1, SP_EL1
```

> 树莓派启动通常从 EL3 开始。Linux 启动代码 `head.S` 会处理降级。

---

## 11.7 实验要点

| 实验 | 内容 | 平台 |
|------|------|------|
| 11-1 | 切换到 EL1 | QEMU |
| 11-2 | 建立异常向量表 | QEMU |
| 11-3 | 寻找触发异常的指令 | Pi4B/QEMU |
| 11-4 | 解析数据异常的信息（ESR/FAR） | QEMU |

---

## 11.8 易错点清单

1. **向量表没对齐** → VBAR 要求至少 2048 字节对齐（`.align 11`）。
2. **忘记保存 X0-X30** → 硬件只存 ELR/SPSR，通用寄存器必须手动压栈。
3. **ERET 之前没恢复 SP** → 如果处理中改了 SP，ERET 后 SP 错误。
4. **在 EL1 误用 SP_EL0** → 向量表前 4 项用 SP0，后 12 项用 SPx，选错会栈混乱。
5. **忘读 ESR/FAR** → 同步异常原因在 ESR_ELx，不读无法诊断。

---

## 书中思考题（自测）

1. 同步异常和异步异常的区别？各举一例。
2. 向量表有多少项？每项多大？为什么要 2048 字节对齐？
3. 硬件自动保存什么？软件需要保存什么？
4. ESR_EL1 和 FAR_EL1 分别存什么？
5. ERET 做了什么操作？

**参考答案：**

1. 同步=当前指令触发（SVC/页错误），PC精确；异步=与当前指令无关（IRQ/SError），PC指向下一条。  
2. **16 项**，每项 **128 字节**；2048 对齐 = 16×128，VBAR 硬件要求。  
3. 硬件存 **ELR(PC) + SPSR(PSTATE)**；软件存 **X0-X30 通用寄存器**。  
4. ESR_EL1=**异常综合征**（原因分类 EC）；FAR_EL1=**故障虚拟地址**。  
5. 原子恢复 **ELR→PC** 和 **SPSR→PSTATE**，切回原 EL。

---

上一章 [Ch10 内联汇编](../../chapter-10-gcc-inline-asm/) · 下一章 [Ch12 中断处理](../../chapter-12-interrupt-handling/) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md)
