# ARM64 启动汇编阶段：U-Boot → head.S → MMU

> 来源: Bootlin ARM64 Training
> 对标旧书: ULK3 Ch2 (x86 启动, 已过时)

---

## ARM64 内核启动全景

```
┌──────────────────────────────────────────────────────┐
│ 1. ROM Bootloader / U-Boot                            │
│    加载 kernel Image + dtb 到内存，跳转               │
├──────────────────────────────────────────────────────┤
│ 2. arch/arm64/kernel/head.S (汇编头)  ← 本文件       │
│    设置初始页表、开启 MMU、跳转 C 代码                │
├──────────────────────────────────────────────────────┤
│ 3. start_kernel() (init/main.c)      → 下一文件       │
│    初始化所有子系统、调度器、中断、驱动               │
├──────────────────────────────────────────────────────┤
│ 4. rest_init() → kernel_init()       → 下一文件       │
│    挂载 rootfs、执行 /sbin/init                       │
└──────────────────────────────────────────────────────┘
```

---

## 阶段 1: U-Boot → 内核入口

```
U-Boot 将内核加载到内存后跳转:
  x0 = dtb 地址 (设备树在内存中的位置)
  x1-x3 = 0
  x4 = 0 (或预留)
  PC = 内核入口地址 (Image 头中的跳转指令)

# ARM64 Image 头 (arch/arm64/kernel/head.S)
# 前 8 字节: "ARM\x64" magic
# 接下来: 跳转指令 + Image 头信息 (text_offset, image_size)
```

### ARM64 Image vs x86 bzImage

| 特性 | x86 bzImage | ARM64 Image |
|------|-------------|-------------|
| 格式 | 压缩内核 (gzip) | 未压缩 (或 Image.gz) |
| 入口 | setup.S → head.S | head.S 直接执行 |
| 引导协议 | real mode → protected mode | 直接在 EL1/EL2 执行 |
| 设备描述 | ACPI | Device Tree (DTB) |
| 内存映射 | e820 | DTB /memory 节点 |

---

## 阶段 2: head.S 汇编初始化

```asm
// arch/arm64/kernel/head.S
__primary_entry:
    bl      preserve_boot_args      // 保存 x0-x3 (dtb 地址等) 到 __boot_args
    bl      el2_setup               // 检查异常等级，降级到 EL1
    bl      set_cpu_boot_mode_flag  // 记录启动模式 (EL2/EL1)
    bl      __create_page_tables    // 创建初始页表 (恒等映射 + 内核映射)
    bl      __cpu_setup             // 配置 SCTLR_EL1 (MMU/cache 控制寄存器)
    bl      __primary_switch        // 开启 MMU，跳转到 C 代码

__primary_switch:
    bl      __enable_mmu            // 设置 TTBR0/TTBR1，开启 MMU (SCTLR_EL1.M=1)
    ldr     x8, =__primary_switched
    br      x8                      // 跳转到 C 代码 (__primary_switched)
```

### EL2 → EL1 降级

```asm
// el2_setup (简化)
el2_setup:
    mrs     x0, CurrentEL           // 读取当前异常等级
    cmp     x0, #PSR_MODE_EL2t      // 是否在 EL2?
    b.ne    1f                      // 不在 EL2, 跳过

    // 在 EL2: 配置虚拟化扩展
    // 设置 HCR_EL2 (Hypervisor 配置寄存器)
    // 配置 VTCR_EL2 (Stage-2 页表控制)
    // 如果需要 KVM, 保存 EL2 状态

    // 降级到 EL1
    mov     x0, #PSR_MODE_EL1h      // SP_EL1 + EL1h
    msr     spsr_el2, x0            // 设置返回后的 PSTATE
    msr     elr_el2, lr             // 设置返回地址
    eret                            // 异常返回 → 降到 EL1
1:
    ret
```

**为什么需要降级？** Linux 内核运行在 EL1。如果在 EL2 启动（Hypervisor 模式），需要先降级。如果需要 KVM 虚拟化，EL2 的状态会被保存供 Hypervisor 使用。

### 创建初始页表

```asm
// __create_page_tables (简化)
// 创建两个映射:
// 1. 恒等映射 (identity mapping): 物理地址 = 虚拟地址
//    用于 MMU 开启瞬间, PC 指向的代码继续可执行
// 2. 内核映射: 物理地址 → 内核虚拟地址 (PAGE_OFFSET + 物理地址)

// ARM64 页表层级:
// PGD (Level 0) → PUD (Level 1) → PMD (Level 2) → PTE (Level 3)
// 4KB 页: 39-bit 或 48-bit 虚拟地址
// 64KB 页: 42-bit 虚拟地址
```

### 开启 MMU

```asm
// __enable_mmu
__enable_mmu:
    // 设置页表基址寄存器
    msr     ttbr0_el1, x0           // 用户空间页表基址 (恒等映射用)
    msr     ttbr1_el1, x1           // 内核空间页表基址

    // 配置 TCR_EL1 (Translation Control Register)
    // 设置 TLB 缓存策略、ASID 宽度、页大小等

    // 开启 MMU + cache
    mrs     x0, sctlr_el1
    orr     x0, x0, #SCTLR_ELx_M    // M=1: 开启 MMU
    orr     x0, x0, #SCTLR_ELx_C    // C=1: 开启 D-cache
    msr     sctlr_el1, x0
    isb                             // 指令同步屏障
    br      __primary_switched       // 跳转到 C 代码
```

---

## 恒等映射的必要性

```
MMU 开启前: PC = 物理地址 0x80000 (内核加载位置)
              ↓
MMU 开启瞬间: 虚拟地址 = 物理地址 (页表翻译生效)
              ↓
如果没有恒等映射: PC 指向的虚拟地址没有页表项 → Page Fault → 崩溃
              ↓
恒等映射保证: 物理地址 0x80000 → 虚拟地址 0x80000 (同一地址)
              ↓
MMU 开启后: 代码继续执行, 之后跳转到内核虚拟地址 (PAGE_OFFSET + 0x80000)
```

---

## 与旧书差异

| ULK3 (x86 启动) | ARM64 启动 |
|-----------------|-----------|
| 16 位 real mode → 32 位保护模式 | EL2 → EL1 直接降级 |
| BIOS → bootloader → setup.S → head.S | U-Boot → head.S (无 real mode) |
| `cr3` 页表基址 | `TTBR0_EL1` / `TTBR1_EL1` |
| `e820` 内存映射 | DTB 中的 `/memory` 节点 |
| ACPI 硬件描述 | Device Tree |
| GDT/IDT 设置 | VBAR_EL1 向量表设置 |

---

## HFT 关联

| 启动阶段 | HFT 关注 |
|---------|---------|
| U-Boot bootargs | isolcpus, nohz_full 等参数在此传递 |
| EL2 → EL1 | KVM 虚拟化对 HFT 通常是开销，不用 |
| 页表设置 | 大页 (2MB/1GB) 减少 TLB miss |
| MMU 开启 | 启动早期延迟不影响交易，但理解流程有助于调试 |

---

## 自测题

<details>
<summary>Q1: ARM64 内核启动时为什么要先从 EL2 降级到 EL1？</summary>

Linux 内核运行在 EL1 (内核态)。如果系统在 EL2 启动（Hypervisor 模式），el2_setup() 会配置虚拟化扩展并降级到 EL1。如果不降级直接运行在 EL2，某些系统寄存器的访问权限不同（如 CNTV_CTL_EL0 只有在 EL1 配置 HCR_EL2 后才可用），会导致内核行为异常。如果需要 KVM，EL2 状态会被保存供 Hypervisor 使用。
</details>

<details>
<summary>Q2: head.S 中创建的"恒等映射"是什么？为什么需要？</summary>

恒等映射 (identity mapping) 是物理地址 = 虚拟地址的映射。开启 MMU 的瞬间，PC 指向的物理地址必须同时有虚拟地址映射，否则 CPU 取下一条指令时会页错误。恒等映射确保 MMU 开启后当前执行的代码继续可用，直到跳转到内核虚拟地址空间的代码（PAGE_OFFSET + 物理地址）。
</details>

<details>
<summary>Q3: ARM64 和 x86 在启动阶段的最大区别是什么？</summary>

x86 启动需要从 16 位 real mode → 32 位保护模式 → 64 位长模式，经过多次模式切换。ARM64 直接在 EL1 或 EL2 启动，已经是 64 位模式，只需 EL2→EL1 降级（如果在 EL2 启动）。ARM64 没有 BIOS，用 U-Boot 和 Device Tree 替代 x86 的 BIOS 和 ACPI。
</details>

---

## 交叉引用

- [02-start-kernel-init.md](./02-start-kernel-init.md) — start_kernel() C 代码初始化
- [chapter-06-arm64-architecture](../../chapter-06-arm64-architecture/) — ARM64 架构详解
- [chapter-09-bootloader-build](../../chapter-09-bootloader-build/) — U-Boot 与构建系统
