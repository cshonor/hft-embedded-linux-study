# §14.7 实验要点

> **来源：** [Ch14 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

本章 7 个实验：建立恒等映射、MMU 排错、页表转储、修改页面属性、汇编建恒等映射+开 MMU、LDXR/STXR 在 MMU 下行为、AT 指令。从基础页表填充到高级调试技巧，完整覆盖 MMU 实操。

## 核心要点

| 实验 | 内容 | 平台 | 关键技能 |
|------|------|------|----------|
| 14-1 | 建立恒等映射 | QEMU | 页表填充 + MMU 开启 |
| 14-2 | 为什么 MMU 无法运行（排错） | QEMU | 常见 MMU 错误诊断 |
| 14-3 | 页表转储功能 | QEMU | 遍历 4 级页表打印映射 |
| 14-4 | 修改页面属性导致死机 | QEMU | 属性设置与 fault 处理 |
| 14-5 | 汇编建恒等映射+开 MMU | QEMU | 纯汇编 MMU 初始化 |
| 14-6 | LDXR/STXR 在 MMU 下行为 | QEMU | 独占监视器与 MMU 交互 |
| 14-7 | AT 指令（地址翻译） | QEMU | 软件模拟地址翻译 |

### 实验 14-1：建立恒等映射

#### 页表填充代码

```c
// 恒等映射：VA=0x60000000 → PA=0x60000000
// 使用 2MB Block descriptor（L2 层），省 L3 级

#define VA_START    0x60000000
#define BLOCK_SIZE  0x200000   // 2MB

extern uint64_t l0_table[];    // L0 页表 (4KB)
extern uint64_t l1_table[];    // L1 页表 (4KB)
extern uint64_t l2_table[];    // L2 页表 (4KB)

void setup_identity_mapping(void) {
    // L0[VA[47:39]] → 指向 L1 表
    // VA[47:39] = 0x60000000 >> 39 = 0x00 → L0[0]
    l0_table[0] = ((uint64_t)l1_table) | PTE_TYPE_TABLE;

    // L1[VA[38:30]] → 指向 L2 表
    // VA[38:30] = (0x60000000 >> 30) & 0x1FF = 0x01 → L1[1]
    l1_table[1] = ((uint64_t)l2_table) | PTE_TYPE_TABLE;

    // L2[VA[29:21]] → Block descriptor (2MB)
    // VA[29:21] = (0x60000000 >> 21) & 0x1FF = 0x00 → L2[0]
    // PA = 0x60000000 (恒等映射)
    l2_table[0] = (VA_START)
        | PTE_TYPE_BLOCK           // Block descriptor
        | (MT_NORMAL_WB << 2)      // AttrIndx=1 (Normal-WB)
        | (0b11 << 8)              // SH=Inner Shareable
        | (1 << 10)                // AF=1
        | (0 << 11)                // nG=0 (全局)
        | (0 << 54)                // UXN=0 (可执行)
        | (0 << 53);               // PXN=0 (可执行)

    // 如果需要映射多个 2MB block，循环填充
    for (int i = 1; i < 4; i++) {  // 映射 4×2MB = 8MB
        l2_table[i] = (VA_START + i * BLOCK_SIZE)
            | PTE_TYPE_BLOCK
            | (MT_NORMAL_WB << 2)
            | (0b11 << 8)
            | (1 << 10);
    }
}
```

#### 汇编入口（实验 14-5 简化版）

```asm
.section .text.boot
.global _start
_start:
    // 设置栈
    ldr x0, =stack_top
    mov sp, x0

    // 建立恒等映射（C 函数）
    bl setup_identity_mapping

    // 设 MAIR_EL1
    ldr x0, =0x00FF0444   // Device | Normal-WB | Normal-NC
    msr mair_el1, x0

    // 设 TCR_EL1
    ldr x0, =(16 | (16 << 16) | (0b01 << 8) | (0b01 << 10) | (0b11 << 12))
    msr tcr_el1, x0

    // 设 TTBR0_EL1
    adrp x0, l0_table
    msr ttbr0_el1, x0

    // clean cache
    dc civac, x0
    dsb sy

    // 开 MMU
    mrs x0, sctlr_el1
    orr x0, x0, #1
    msr sctlr_el1, x0
    isb

    // 现在在虚拟地址空间了，跳转到 C 代码
    bl main

hang:
    wfe
    b hang
```

#### 编译运行

```bash
# 编译
aarch64-linux-gnu-gcc -c -o start.o start.S
aarch64-linux-gnu-gcc -c -o mmu.o mmu.c
aarch64-linux-gnu-ld -T linker.ld -o kernel.elf start.o mmu.o
aarch64-linux-gnu-objcopy -O binary kernel.elf kernel8.img

# QEMU 运行
qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 128M \
    -kernel kernel8.img -nographic
```

### 实验 14-3：页表转储

```c
// 遍历 4 级页表，打印所有有效映射
void dump_page_tables(uint64_t *l0) {
    for (int i0 = 0; i0 < 512; i0++) {
        if (!(l0[i0] & 1)) continue;  // 无效
        uint64_t *l1 = (uint64_t *)(l0[i0] & 0x0000FFFFFFFFF000ULL);
        for (int i1 = 0; i1 < 512; i1++) {
            if (!(l1[i1] & 1)) continue;
            if (l1[i1] & 0x2) {  // Block (1GB)
                uint64_t va = ((uint64_t)i0 << 39) | ((uint64_t)i1 << 30);
                uint64_t pa = l1[i1] & 0xFFFFC0000000ULL;
                printf("VA=%p → PA=%p (1GB Block)\n", va, pa);
            } else {  // Table → L2
                uint64_t *l2 = (uint64_t *)(l1[i1] & 0x0000FFFFFFFFF000ULL);
                for (int i2 = 0; i2 < 512; i2++) {
                    if (!(l2[i2] & 1)) continue;
                    if (l2[i2] & 0x2) {  // Block (2MB)
                        uint64_t va = ((uint64_t)i0 << 39) | ((uint64_t)i1 << 30)
                                    | ((uint64_t)i2 << 21);
                        uint64_t pa = l2[i2] & 0xFFFFFFE00000ULL;
                        printf("VA=%p → PA=%p (2MB Block)\n", va, pa);
                    }
                    // ... L3 遍历省略
                }
            }
        }
    }
}
```

### 实验 14-7：AT 指令

```c
// AT 指令：软件模拟地址翻译，不触发实际访存
void test_at_instruction(uint64_t va) {
    uint64_t par;

    // at s1e1r, x0 — 将 x0 的 VA 翻译为 PA，结果存入 PAR_EL1
    asm volatile("at s1e1r, %0" :: "r"(va));
    asm volatile("mrs %0, par_el1" : "=r"(par));

    if (par & 1) {
        // bit0=1 表示翻译失败（fault）
        printf("VA=%p: Translation FAILED (FST=0x%x)\n",
               va, (par >> 6) & 0x3F);
    } else {
        // bit0=0 表示翻译成功
        uint64_t pa = (par & 0x0000FFFFFFFFF000ULL) | (va & 0xFFF);
        printf("VA=%p → PA=%p (AT translate OK)\n", va, pa);
    }
}
```

### 实验递进关系

```
14-1 (基础页表) → 14-2 (排错) → 14-3 (转储) → 14-4 (属性)
                                                    |
14-5 (汇编版) ← 14-6 (独占监视器) ← 14-7 (AT 指令) ←─┘
```

## HFT 关联

实验 14-1 和 14-5 是 HFT 裸金属开发的基础——没有 MMU 就无法使用虚拟地址，也无法配置内存属性。实验 14-3（页表转储）是调试 HFT 内存映射的重要工具——可以验证 MMIO 区域是否正确映射为 Device 属性。实验 14-4（属性导致死机）帮助理解错误属性设置的症状，避免在实际 HFT 系统中犯同样错误。

## 自测题

1. **实验 14-1 中，建立恒等映射需要填充哪几级页表？**

<details>
<summary>答案</summary>

需要填充 **L0 → L1 → L2 → L3** 全部 4 级（如果用 4KB 页）。也可以在 L1 或 L2 用 Block descriptor 省略下级——如用 L2 Block（2MB）只需 L0 → L1 → L2 三级。恒等映射的 VA=PA，如 VA=0x60000000 → PA=0x60000000。
</details>

2. **实验 14-2 中 MMU 无法运行，列出 3 个最可能的原因。**

<details>
<summary>答案</summary>

1. **没有恒等映射** → 开 MMU 后取指 page fault
2. **忘记 ISB** → 流水线中旧指令执行，行为不可预测
3. **页表没 clean cache** → MMU walker 读到旧页表
4. （附加）MAIR/TCR 设置错误、TTBR 指向错误地址
</details>

3. **实验 14-7 中 AT 指令的作用是什么？**

<details>
<summary>答案</summary>

AT（Address Translate）指令在**软件中模拟 MMU 地址翻译**，不实际访问内存。如 `at s1e1r, x0` 将 x0 的 VA 翻译为 PA 并存入 PAR_EL1。用于调试——可以检查某个 VA 映射到哪个 PA，验证页表是否正确，而不触发实际的访存异常。
</details>

4. **用 2MB Block descriptor 建立恒等映射，需要几级页表？比 4KB 页省几级？**

<details>
<summary>答案</summary>

用 2MB Block（L2 Block descriptor）需要 **L0 → L1 → L2** 共 3 级，不需要 L3。比 4KB 页（4 级）省 1 级。页表内存从 L0+L1+L2+L3（4 页=16KB）减少到 L0+L1+L2（3 页=12KB），且每个 L2 Block 映射 2MB，效率更高。
</details>

## 参考与延伸

- [§14.6 开 MMU 流程](06-enable-mmu.md) — 实验 14-1/14-5 的核心步骤
- [§14.8 易错点](08-pitfalls.md) — 实验 14-2 排错的参考
- [§14.3 页表项格式](03-descriptor-format.md) — 实验 14-3 页表转储的基础
