# 7.6 Linux 启动汇编分析（大作业）

> 来源：§7.6 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

分析 Linux 内核启动汇编代码 `head.S`，综合应用前 6 章学到的所有指令知识。这是检验学习成果的大作业。

## 核心要点

### Linux ARM64 启动流程概览

```
ROM/BL → bootloader → kernel head.S → start_kernel()

head.S 执行流程（arch/arm64/kernel/head.S）：
  1. 检查当前 EL，从 EL3/EL2 降到 EL1
  2. 设置初始栈（SP）
  3. 建立恒等映射（Identity Mapping, VA=PA）
  4. 建立内核镜像映射（Kernel Image Mapping）
  5. 开启 MMU
  6. 跳转到虚拟地址执行
  7. 调用 start_kernel()（C 代码）
```

### 关键代码段分析

#### 1. EL 检查与降级

```asm
// arch/arm64/kernel/head.S 简化版
ENTRY(stext)
    // 检查当前 EL
    mrs x0, CurrentEL          // 读当前异常等级
    lsr x0, x0, #2             // EL 在 bit[3:2]
    cmp x0, #3                 // EL3?
    b.eq 1f                    // 是 → 从 EL3 降级

    cmp x0, #2                 // EL2?
    b.eq 2f                    // 是 → 从 EL2 降级

    b 3f                       // EL1 → 直接继续

1:  // EL3 → EL2
    // 配置 SCR_EL3（安全配置）
    mov x0, #(1 << 0) | (1 << 10) | (1 << 18)  // NS | RW | ST
    msr scr_el3, x0
    msr cptr_el3, xzr          // 禁用陷阱
    // 设置 EL2 返回地址
    adr_l x0, 2f
    msr elr_el3, x0
    eret                       // 降到 EL2

2:  // EL2 → EL1
    // 配置 HCR_EL2（虚拟化配置）
    mov_q x0, (1 << 31)        // RW=1（AArch64 EL1）
    msr hcr_el2, x0
    // 设置 EL1 返回地址
    adr_l x0, 3f
    msr elr_el2, x0
    eret                       // 降到 EL1
```

**指令综合**：MRS（读系统寄存器）、LSR（移位）、CMP+B.cond（比较跳转）、MSR（写系统寄存器）、ADR（PC相对地址）、ERET（异常返回）。

#### 2. ADRP 获取页表地址

```asm
    // 获取初始页表地址
    adrp x0, init_pg_dir       // 初始页表（恒等映射）
    adrp x1, swapper_pg_dir    // 内核页表（最终映射）
    // ADRP 获取 4KB 页对齐地址
    // ADD :lo12: 补全页内偏移（这里页表本身页对齐，不需 ADD）

    // 设置 TTBR
    msr ttbr0_el1, x0          // 用户空间页表（初始用恒等映射）
    msr ttbr1_el1, x1          // 内核空间页表
```

**指令综合**：ADRP（PC 相对页地址）、MSR（写系统寄存器）。

#### 3. 开启 MMU

```asm
    // 配置 TCR_EL1（Translation Control Register）
    mov_q x0, TCR_VALUE
    msr tcr_el1, x0

    // 配置 MAIR_EL1（Memory Attribute Indirection Register）
    mov_q x0, MAIR_VALUE       // 设置内存属性
    msr mair_el1, x0

    // 刷新 TLB 和缓存
    tlbi vmalle1is             // 刷新整个 TLB
    dsb ish                     // 等待 TLB 刷新完成

    // 读取 SCTLR_EL1，开启 MMU
    mrs x0, sctlr_el1
    orr x0, x0, #1             // set MMU enable bit (M=1)
    orr x0, x0, #(1 << 2)      // set D-cache enable (C=1)
    msr sctlr_el1, x0
    isb                         // 关键！冲刷流水线确保 MMU 生效

    // 跳转到虚拟地址
    ldr x8, =__primary_switched
    br x8                       // 间接跳转到虚拟地址
```

**指令综合**：MRS/MSR（系统寄存器读写）、ORR（位设置）、TLBI（TLB 刷新）、DSB（数据同步屏障）、ISB（指令同步屏障）、LDR =（地址加载）、BR（寄存器跳转）。

#### 4. 栈设置

```asm
__primary_switched:
    // 设置栈指针
    adr_l x0, init_thread_union
    add sp, x0, #THREAD_SIZE
    // SP 指向线程栈顶（THREAD_SIZE 通常是 16KB 或 32KB）
    // 自动 16 字节对齐（THREAD_SIZE 是 16 的倍数）

    // 清零 BSS 段
    adr_l x0, __bss_start
    adr_l x1, __bss_stop
1:
    str xzr, [x0], #8          // 清零 8 字节，后变基
    cmp x0, x1
    b.lo 1b                    // 未到末尾 → 继续

    // 调用 start_kernel（C 函数）
    bl start_kernel
```

**指令综合**：ADR（地址加载）、ADD（栈指针计算）、STR（BSS 清零，后变基 `[x0], #8`）、CMP+B.LO（循环判断）、BL（函数调用）。

### 恒等映射（Identity Mapping）详解

```
为什么要恒等映射？

开 MMU 前：
  PC = 0x40000000（物理地址）
  取指直接从物理地址读取

开 MMU 后：
  PC = 0x40000000（虚拟地址）
  取指需经过 MMU 翻译：VA 0x40000000 → PA ???
  如果没有映射 → Page Fault → 死机！

恒等映射：VA = PA
  页表中建立 0x40000000(VA) → 0x40000000(PA) 的映射
  开 MMU 后，PC=0x40000000 仍翻译为 0x40000000 → 正常执行

跳转到虚拟地址后：
  PC = 0xFFFF000000000000 + offset（内核虚拟地址）
  页表中建立内核虚拟地址 → 物理地址的映射
  恒等映射可以移除（不再需要 VA=PA）
```

## 与 C 的对照

```c
// start_kernel() 之前的代码都是汇编
// start_kernel() 是第一个 C 函数
asmlinkage __visible void __init start_kernel(void) {
    // 此时 MMU 已开，栈已设，可以正常用 C
    setup_arch(&command_line);
    // ...
}
```

## 常见错误

1. **开 MMU 后忘记 ISB**：流水线中残留旧指令 → 崩溃。
2. **恒等映射范围不足**：只映射了当前 PC 附近，跳转到其他地址 → Page Fault。
3. **页表属性错误**：代码段用 Normal Non-cacheable → 性能暴跌。

## HFT 关联

理解内核启动对 HFT 的意义：
- 内核启动的 MMU 配置决定了后续所有内存访问的属性 → 影响 cache 行为
- 恒等映射和虚拟地址切换是理解内核地址空间的基础
- HFT 优化需要理解内核的页表布局（哪些是 Normal/Device）
- 启动代码中的屏障使用是正确内存序的最佳教材

## 自测题

1. Linux 启动时为什么要先建恒等映射再开 MMU？
<details><summary>答案</summary>
开 MMU 前 PC 是物理地址。开 MMU 后，取指需要经过 MMU 翻译（VA→PA）。如果没有恒等映射（VA=PA 的映射），MMU 翻译当前 PC 地址会 page fault，CPU 挂死。恒等映射保证开 MMU 后取指继续正常工作，直到跳转到真正的虚拟地址。
</details>

2. 开 MMU 后为什么必须跟 ISB？
<details><summary>答案</summary>
开 MMU（MSR SCTLR_EL1）改变了地址翻译行为。但流水线中可能有之前取的指令（基于旧 MMU 状态）。ISB 冲刷流水线，强制重新取指，确保后续指令在新的 MMU 状态下执行。不跟 ISB 可能导致取到旧地址翻译的指令，行为不可预测。
</details>

3. 启动代码中从 EL3 降到 EL1 的过程是什么？
<details><summary>答案</summary>
1. 在 EL3 配置 SCR_EL3（允许 HVC，设为非安全）
2. 设置 ELR_EL3 = EL2 入口地址，SPSR_EL3 = 目标 PSTATE
3. ERET → 降到 EL2
4. 在 EL2 配置 HCR_EL2（设为非虚拟化模式）
5. 设置 ELR_EL2 = EL1 入口地址
6. ERET → 降到 EL1
7. 在 EL1 设置 VBAR/SP，开始正常执行
</details>

4. head.S 中 BSS 段清零为什么用 STR+后变基 `[x0], #8` 而不是普通 STR？
<details><summary>答案</summary>
后变基 `STR XZR, [X0], #8` 一步完成存储和指针更新：先写 *X0=0，然后 X0+=8。如果用普通 STR 需要额外的 ADD 指令更新指针，循环体多一条指令。后变基是 ARM 循环清零/拷贝的标准优化模式。每步只 1 条指令（加 CMP+B.LO 共 2 条），效率最高。
</details>

5. 为什么 TTBR0 和 TTBR1 分别设置？
<details><summary>答案</summary>
AArch64 使用两个页表基址寄存器：
- TTBR0_EL1：用户空间虚拟地址（低地址段，如 0x0000...0000 ~ 0x00FF...FFFF）
- TTBR1_EL1：内核空间虚拟地址（高地址段，如 0xFFFF...0000 ~ 0xFFFF...FFFF）

由 TCR_EL1 的 T0SZ/T1SZ 决定地址分割点。启动时 TTBR0 设为恒等映射页表（VA=PA），TTBR1 设为内核页表（高地址映射）。切换到虚拟地址执行后，TTBR0 可以改为用户进程页表。
</details>

## 参考与延伸

- 原书 §7.6
- [6.1 ADR/ADRP](../../chapter-06-a64-other-instructions/notes/01-adr-adrp.md)
- [6.3 MRS/MSR](../../chapter-06-a64-other-instructions/notes/03-mrs-msr.md)
- [Ch14 MMU 开启流程](../../chapter-14-memory-management/notes/section-0-本章完整概述.md)
