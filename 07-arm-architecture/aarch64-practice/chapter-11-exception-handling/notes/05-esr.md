# §11.5 异常综合征（ESR）

> **来源：** [Ch11 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

同步异常发生时，ESR_ELx 寄存器包含异常原因的分类编码（EC），FAR_ELx 保存触发数据异常的虚拟地址。通过读 ESR + FAR 可以精确定位异常原因。

## ESR_ELx 寄存器格式

```
 31                    26 25                        0
┌────────────────────────┬───────────────────────────┐
│        EC (6 bits)      │       ISS (25 bits)        │
└────────────────────────┴───────────────────────────┘
```

| 字段 | 位宽 | 含义 |
|------|------|------|
| EC (Exception Class) | bit[31:26] | 异常类型分类 |
| ISS (Instruction Specific Syndrome) | bit[24:0] | 异常详细信息（因 EC 而异） |

### 读取 EC 字段

```asm
mrs x0, ESR_EL1         // 读异常综合征
lsr x1, x0, #26         // EC 在 bit[31:26]，右移 26 位
// x1 = EC 值
```

### 读取 ISS 字段

```asm
mrs x0, ESR_EL1
and x1, x0, #0x1FFFFFF  // ISS 在 bit[24:0]，掩码 25 位
// x1 = ISS 值
```

## 常见 EC 值

| EC 值 | 含义 | 典型场景 |
|-------|------|---------|
| 0x15 | SVC（AArch64） | 系统调用 `svc #0` |
| 0x16 | HVC | Hypervisor 调用 |
| 0x17 | SMC | Secure Monitor 调用 |
| 0x18 | MRS/MSR（EL0） | 用户态访问系统寄存器 |
| 0x20 | 指令中止（EL0） | 用户态取指页错误 |
| 0x21 | 指令中止（EL1） | 内核态取指页错误 |
| 0x24 | 数据中止（EL0） | 用户态访存页错误 |
| 0x25 | 数据中止（EL1） | 内核态访存页错误 |
| 0x22 | 对齐错误（EL0） | 用户态未对齐访问 |
| 0x26 | 对齐错误（EL1） | 内核态未对齐访问 |
| 0x00 | Unknown | 未知原因 |
| 0x0E | Illegal Execution State | 非法执行状态 |

### 数据中止（EC=0x24/0x25）的 ISS 格式

```
 ISS bit[24:0]:
  24    6 5  0
  ┌──────┬─┬──┐
  │DFSC  │W│  │
  └──────┴─┴──┘
```

| ISS 字段 | 位 | 含义 |
|---------|-----|------|
| WnR | bit[6] | Write not Read（1=写，0=读） |
| DFSC | bit[5:0] | 数据故障状态码 |

| DFSC 值 | 含义 |
|---------|------|
| 0b000100 | Translation fault level 0 |
| 0b000101 | Translation fault level 1 |
| 0b000110 | Translation fault level 2 |
| 0b000111 | Translation fault level 3 |
| 0b001001 | Permission fault level 1 |
| 0b001011 | Permission fault level 3 |
| 0b001000 | Synchronous external abort |

## ESR vs FAR

| 寄存器 | 作用 | 何时有效 |
|--------|------|---------|
| ESR_ELx | 异常原因分类（EC + ISS） | 所有同步异常 |
| FAR_ELx | 触发异常的虚拟地址 | 数据中止/指令中止 |
| ELR_ELx | 异常返回地址 | 所有异常 |

### 诊断流程

```c
void sync_handler(void) {
    u64 esr, far, elr;
    asm volatile("mrs %0, ESR_EL1" : "=r"(esr));
    asm volatile("mrs %0, FAR_EL1" : "=r"(far));
    asm volatile("mrs %0, ELR_EL1" : "=r"(elr));

    u32 ec = esr >> 26;    // 提取 EC

    switch (ec) {
    case 0x15:  // SVC
        printf("SVC call, immediate=%d\n", esr & 0x1FFFFFF);
        break;
    case 0x24:  // Data Abort from EL0
        printf("Page fault at %p, addr=%p, %s\n",
               elr, far, (esr & (1<<6)) ? "write" : "read");
        break;
    case 0x25:  // Data Abort from EL1
        printf("KERNEL BUG: data abort at %p, addr=%p\n", elr, far);
        break;
    default:
        printf("Unknown exception, EC=0x%x, ELR=%p\n", ec, elr);
    }
}
```

## SVC 系统调用号提取

```asm
// SVC 的 ISS 低 16 位是立即数（系统调用号）
mrs x0, ESR_EL1
and x0, x0, #0xFFFF      // 提取 SVC 立即数
// x0 = 系统调用号

// 通常用 X8 寄存器传系统调用号更常见
// ESR 的 SVC 立即数只是 SVC 指令编码中的值
```

## HFT 关联

在 HFT 裸金属开发中，ESR/FAR 是调试页错误的第一工具。交易系统访问未映射的内存地址会导致同步异常，通过读 ESR 判断是取指错误（0x20）还是数据访问错误（0x24），再读 FAR 获取具体地址，可以快速定位是哪行代码访问了非法地址。在内核态（EL1）发生数据中止（EC=0x25）通常意味着内核 bug，需要立即处理。

## 自测题

1. **ESR_EL1 的 EC 字段在哪些位？怎么提取？**
<details><summary>答案</summary>
EC 在 **bit[31:26]**（最高 6 位）。提取：`mrs x0, ESR_EL1; lsr x1, x0, #26`，x1 即 EC 值。EC 值决定异常类型（SVC=0x15, 数据中止=0x24/0x25, 指令中止=0x20/0x21 等）。
</details>

2. **SVC 系统调用的 EC 值是多少？如何从 ESR 中获取 SVC 指令的立即数？**
<details><summary>答案</summary>
SVC 的 EC = **0x15**。SVC 指令的立即数在 ISS（低 25 位，bit[24:0]）中。用 `and x1, x0, #0x1FFFFFF` 提取 ISS。实际上 SVC 的立即数只有低 16 位有效（`svc #imm16`）。但 Linux 通常用 X8 寄存器传系统调用号而非 SVC 立即数。
</details>

3. **FAR_ELx 在什么情况下有效？什么情况下无效？**
<details><summary>答案</summary>
FAR 在**数据中止**（访存异常，如 EC=0x24/0x25）和**指令中止**（EC=0x20/0x21）时有效，保存触发异常的虚拟地址。但在 SVC、未定义指令等非访存异常中，FAR 的值**无效**（可能是上一次异常的残留值），不应使用。
</details>

4. **数据中止的 ISS 中 WnR 位（bit[6]）有什么用？**
<details><summary>答案</summary>
WnR（Write not Read）指示是写操作还是读操作触发了异常。WnR=1 表示写操作（如 `str x0, [bad_addr]`），WnR=0 表示读操作（如 `ldr x0, [bad_addr]`）。调试时用于判断是写入还是读取非法地址。
</details>

5. **如何在异常处理中区分 Translation fault 和 Permission fault？**
<details><summary>答案</summary>
读 ESR 的 ISS 中 DFSC 字段（bit[5:0]）。Translation fault（页表不存在）的 DFSC = 0b000100~0b000111（level 0-3）。Permission fault（页表存在但权限不足）的 DFSC = 0b001001~0b001011（level 1-3）。Translation fault 需要建立页表映射，Permission fault 需要修改页表权限位。
</details>

## 参考与延伸

- [§11.1 异常类型](01-exception-types.md) — 哪些异常是同步的（才有 ESR）
- [§11.4 硬件保存+软件保存](04-hw-sw-save.md) — 读 ESR 是在保存完现场之后
- [§11.7 实验要点](07-lab.md) — 实验 11-4 解析 ESR/FAR
