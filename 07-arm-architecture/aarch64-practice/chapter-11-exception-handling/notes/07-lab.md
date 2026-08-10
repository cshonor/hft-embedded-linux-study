# §11.7 实验要点

> **来源：** [Ch11 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

本章的 4 个实验：切换到 EL1、建立异常向量表、寻找触发异常的指令、解析数据异常信息（ESR/FAR）。从基础降级到完整异常处理。

## 实验列表

| 实验 | 内容 | 平台 | 关键技能 |
|------|------|------|----------|
| 11-1 | 切换到 EL1 | QEMU | CurrentEL 检测 + ERET 降级 |
| 11-2 | 建立异常向量表 | QEMU | VBAR 设置 + 16 项定义 |
| 11-3 | 寻找触发异常的指令 | Pi4B/QEMU | 读 ELR 回溯触发指令 |
| 11-4 | 解析数据异常的信息（ESR/FAR） | QEMU | ESR EC 字段 + FAR 地址 |

### 实验递进关系

```
11-1 (EL降级) → 11-2 (建向量表) → 11-3 (触发+定位异常) → 11-4 (解析异常原因)
   基础           框架              调试能力               诊断能力
```

---

## 实验 11-1：切换到 EL1

### 目标

检测当前 EL，从 EL2/EL3 降级到 EL1。

### 代码

```asm
.section .text.boot
.global _start
_start:
    mrs x0, CurrentEL
    lsr x0, x0, #2
    cmp x0, #3
    b.eq from_el3
    cmp x0, #2
    b.eq from_el2
    b  in_el1

from_el3:
    mov x0, #0x5b1
    msr SCR_EL3, x0
    mov x0, #0x3c5
    msr SPSR_EL2, x0
    adr x0, from_el2
    msr ELR_EL2, x0
    eret

from_el2:
    mov x0, #(1 << 31)
    msr HCR_EL2, x0
    mov x0, #0x3c5
    msr SPSR_EL2, x0
    adr x0, in_el1
    msr ELR_EL2, x0
    eret

in_el1:
    // 验证当前 EL
    mrs x0, CurrentEL
    lsr x0, x0, #2
    // x0 应该 = 1
```

### 操作步骤

```bash
# QEMU -machine virt 默认从 EL1 启动
qemu-system-aarch64 -M virt -cpu cortex-a57 -kernel benos.elf -nographic

# 如果从 EL2 启动（加 -bios 指定固件）
qemu-system-aarch64 -M virt -cpu cortex-a57 -bios fw.bin -kernel benos.elf -nographic

# GDB 调试
qemu-system-aarch64 -M virt -cpu cortex-a57 -kernel benos.elf -nographic -S -gdb tcp::1234
aarch64-linux-gnu-gdb
(gdb) target remote :1234
(gdb) b _start
(gdb) c
(gdb) info registers CurrentEL
```

---

## 实验 11-2：建立异常向量表

### 目标

设置 VBAR_EL1，定义 16 项向量表，处理 SVC 异常。

### 代码

```asm
// 设置向量表
in_el1:
    adrp x0, vector_table
    add  x0, x0, #:lo12:vector_table
    msr  VBAR_EL1, x0
    isb

// 触发 SVC 测试
    svc #0

// 向量表定义
.align 11
vector_table:
    // 当前 EL SP0 (0x000-0x180)
    .align 7
    b sync_sp0
    .align 7
    b irq_sp0
    .align 7
    b fiq_sp0
    .align 7
    b serror_sp0

    // 当前 EL SPx (0x200-0x380)
    .align 7
    b sync_spx
    .align 7
    b irq_spx
    .align 7
    b fiq_spx
    .align 7
    b serror_spx

    // 低 EL → EL1 AArch64 (0x400-0x580)
    .align 7
    b sync_el0_a64              // ← SVC 从这里进入
    .align 7
    b irq_el0_a64
    .align 7
    b fiq_el0_a64
    .align 7
    b serror_el0_a64

    // 低 EL → EL1 AArch32 (0x600-0x780)
    .align 7
    b sync_el0_a32
    .align 7
    b irq_el0_a32
    .align 7
    b fiq_el0_a32
    .align 7
    b serror_el0_a32

sync_el0_a64:
    // 保存现场
    sub sp, sp, #272
    stp x0, x1, [sp, #0]
    // ...
    // 处理 SVC
    bl do_svc
    // 恢复现场
    ldp x0, x1, [sp, #0]
    add sp, sp, #272
    eret
```

### 验证

```bash
# QEMU 跟踪异常
qemu-system-aarch64 -M virt -cpu cortex-a57 -kernel benos.elf -nographic -d int

# 查看异常跳转日志
# 应看到: SVC from EL0 → VBAR_EL1 + 0x400
```

---

## 实验 11-3：寻找触发异常的指令

### 目标

故意触发一个同步异常（如访问非法地址），读 ELR 找到触发指令。

### 代码

```asm
    // 故意访问非法地址
    ldr x0, =0xDEAD0000        // 未映射的地址
    ldr x1, [x0]               // 触发数据中止

// 在异常处理中
sync_spx:
    sub sp, sp, #272
    stp x0, x1, [sp, #0]

    // 读 ELR 获取触发指令地址
    mrs x0, ELR_EL1
    // x0 = 触发 ldr x1, [x0] 的地址

    // 读 ESR 确认异常类型
    mrs x1, ESR_EL1
    lsr x1, x1, #26
    // x1 = 0x25 (EL1 Data Abort)

    // 读 FAR 获取非法访问地址
    mrs x2, FAR_EL1
    // x2 = 0xDEAD0000

    // 打印 x0, x1, x2 供调试
    bl print_debug

    ldp x0, x1, [sp, #0]
    add sp, sp, #272
    eret
```

---

## 实验 11-4：解析数据异常信息（ESR/FAR）

### 目标

解析 ESR 的 EC 字段和 ISS 子字段，区分不同数据异常原因。

### 代码

```c
void sync_handler_el1(void) {
    u64 esr, far, elr;
    asm volatile("mrs %0, ESR_EL1" : "=r"(esr));
    asm volatile("mrs %0, FAR_EL1" : "=r"(far));
    asm volatile("mrs %0, ELR_EL1" : "=r"(elr));

    u32 ec = esr >> 26;
    u32 iss = esr & 0x1FFFFFF;

    printf("=== Sync Exception ===\n");
    printf("ELR = 0x%llx (triggering instruction)\n", elr);
    printf("ESR = 0x%llx\n", esr);
    printf("  EC  = 0x%x\n", ec);
    printf("  ISS = 0x%x\n", iss);

    switch (ec) {
    case 0x25:  // Data Abort from EL1
        printf("  Type: EL1 Data Abort\n");
        printf("  FAR = 0x%llx (faulting address)\n", far);
        printf("  Access: %s\n", (iss & (1<<6)) ? "Write" : "Read");
        u32 dfsc = iss & 0x3F;
        if (dfsc >= 4 && dfsc <= 7)
            printf("  Reason: Translation fault (level %d)\n", dfsc - 4);
        else if (dfsc >= 9 && dfsc <= 11)
            printf("  Reason: Permission fault (level %d)\n", dfsc - 8);
        break;
    case 0x15:
        printf("  Type: SVC (syscall #%d)\n", iss & 0xFFFF);
        break;
    default:
        printf("  Type: Unknown EC=0x%x\n", ec);
    }
}
```

## HFT 关联

实验 11-2（建立向量表）是裸金属 HFT 的必经之路——没有向量表，任何异常都会导致死机。实验 11-4（解析 ESR/FAR）是调试页错误的利器，HFT 系统中 MMIO 访问错误是最常见的 bug。建议在 QEMU 上完成全部 4 个实验后再上 Pi5。

## 自测题

1. **实验 11-2 中，向量表为什么要用 `.align 11` 对齐？**
<details><summary>答案</summary>
`.align 11` = 2048 字节对齐（2^11）。VBAR 寄存器要求向量表起始地址低 11 位为 0。不对齐的话，`VBAR + offset` 计算出的跳转地址错误，异常时跳到错误位置 → 死机。`.align 7` = 128 字节对齐用于每个表项。
</details>

2. **实验 11-3 中，如何找到触发同步异常的指令？**
<details><summary>答案</summary>
读 **ELR_EL1**，它保存了触发同步异常的指令地址（同步异常的 ELR 指向触发指令本身）。用 `mrs x0, ELR_EL1` 获取地址，然后在反汇编中查找该地址对应的指令。对于数据中止，触发指令是 LDR/STR；对于 SVC，触发指令是 SVC。
</details>

3. **实验 11-4 中，如何区分页错误和对齐错误？**
<details><summary>答案</summary>
读 **ESR_EL1** 的 EC 字段（bit[31:26]）。EC=0x24/0x25 是数据中止（页错误/权限错误），EC=0x22/0x26 是对齐错误。进一步看 ISS 的 DFSC 子字段区分翻译错误和权限错误。
</details>

4. **QEMU 的 `-d int` 选项有什么用？**
<details><summary>答案</summary>
`-d int` 让 QEMU 打印所有异常/中断的日志，包括异常类型、来源 EL、目标 EL、跳转地址（VBAR+offset）。调试异常问题时非常有用——可以看到硬件跳转是否正确，VBAR 是否设置正确。
</details>

5. **实验 11-1 中 QEMU `-machine virt` 默认从哪个 EL 启动？**
<details><summary>答案</summary>
QEMU `-machine virt` 不加 `-bios` 时，直接 `-kernel` 加载的程序从 **EL1** 启动。如果加了 `-bios fw.bin`（如 U-Boot 或 TF-A），可能从 EL2 或 EL3 启动。所以裸机实验中如果不需要测 EL 降级，直接用 `-kernel` 即可从 EL1 开始。
</details>

## 参考与延伸

- [§11.6 EL2→EL1 实验](06-el2-to-el1.md) — 实验 11-1 的详细步骤
- [§11.3 异常向量表](03-vector-table.md) — 实验 11-2 的核心知识
- [§11.5 异常综合征](05-esr.md) — 实验 11-4 的核心知识
