# §15.6 实验要点

> **来源：** [Ch15 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

本章 2 个实验：枚举高速缓存（读 CTR_EL0）和清理高速缓存（dc cvac/civac）。在 Linux 用户态操作 cache 系统寄存器，理解 cache 维护指令的实际效果。

## 核心要点

| 实验 | 内容 | 平台 | 关键技能 |
|------|------|------|----------|
| 15-1 | 枚举高速缓存（读 CTR_EL0 / CSSELR） | Linux | 读 cache 信息寄存器 |
| 15-2 | 清理高速缓存（dc cvac / civac） | Linux | cache 维护指令 |

### 实验 15-1：枚举高速缓存

#### CTR_EL0 寄存器格式

```c
#include <stdio.h>
#include <stdint.h>

void enumerate_cache(void) {
    uint64_t ctr;
    // 读 CTR_EL0（EL0 可读）
    asm volatile("mrs %0, CTR_EL0" : "=r"(ctr));

    // DminLine: D-cache line 最小大小（bit[3:0]）
    // 实际 line size = 4 << DminLine 字节
    int dminline = (int)(ctr & 0xF);
    int d_line_size = 4 << dminline;

    // IminLine: I-cache line 最小大小（bit[19:16]）
    int iminline = (int)((ctr >> 16) & 0xF);
    int i_line_size = 4 << iminline;

    // DIC: I/D cache 独立性（bit[29]）
    // DIC=1: I-cache 和 D-cache 独立（不需要 clean D → invalidate I）
    int dic = (int)((ctr >> 29) & 1);

    // IDC: I/D cache 一致性（bit[28]）
    // IDC=1: 硬件自动维护 I/D 一致性
    int idc = (int)((ctr >> 28) & 1);

    printf("CTR_EL0 = 0x%lx\n", ctr);
    printf("D-cache line size: %d bytes (DminLine=%d)\n", d_line_size, dminline);
    printf("I-cache line size: %d bytes (IminLine=%d)\n", i_line_size, iminline);
    printf("DIC (I/D independent): %d\n", dic);
    printf("IDC (I/D coherent): %d\n", idc);
}
```

#### 运行结果（Pi5 Cortex-A76 示例）

```
CTR_EL0 = 0x8000C004
D-cache line size: 64 bytes (DminLine=4)
I-cache line size: 64 bytes (IminLine=4)
DIC (I/D independent): 1
IDC (I/D coherent): 1
```

> DIC=1 + IDC=1 表示 A76 硬件自动维护 I/D cache 一致性，自修改代码不需要手动 clean D → invalidate I。

### 实验 15-2：清理高速缓存

#### Cache 维护指令

```asm
; Clean（写回脏数据，不丢弃）
dc cvac, x0       ; Clean by VA to PoC（到内存级别）
dc cvau, x0       ; Clean by VA to PoU（到 L2 级别）

; Clean + Invalidate（写回+丢弃）
dc civac, x0      ; Clean+Invalidate by VA to PoC

; Invalidate（丢弃，不写回）
dc ivac, x0       ; Invalidate by VA to PoC

; I-Cache 维护
ic ivau, x0       ; Invalidate I-Cache by VA to PoU
ic iallu          ; Invalidate All I-Cache (local)
ic ialluis        ; Invalidate All I-Cache (Inner Shareable)
```

#### C 代码示例

```c
#include <stdint.h>
#include <stdio.h>

// Cache line 大小（从 CTR_EL0 动态获取）
static int cache_line_size = 0;

void init_cache_line_size(void) {
    uint64_t ctr;
    asm volatile("mrs %0, CTR_EL0" : "=r"(ctr));
    cache_line_size = 4 << (ctr & 0xF);
}

// Clean（写回脏数据到内存）
void cache_clean(void *addr, size_t size) {
    uint64_t va = (uint64_t)addr & ~0x3FULL;
    uint64_t end = ((uint64_t)addr + size + 63) & ~0x3FULL;
    while (va < end) {
        asm volatile("dc cvac, %0" :: "r"(va));
        va += cache_line_size;
    }
    asm volatile("dsb sy");  // 等待 clean 完成
}

// Invalidate（丢弃 cache 行）
void cache_invalidate(void *addr, size_t size) {
    uint64_t va = (uint64_t)addr & ~0x3FULL;
    uint64_t end = ((uint64_t)addr + size + 63) & ~0x3FULL;
    while (va < end) {
        asm volatile("dc ivac, %0" :: "r"(va));
        va += cache_line_size;
    }
    asm volatile("dsb sy");
}

// Flush（Clean + Invalidate）
void cache_flush(void *addr, size_t size) {
    uint64_t va = (uint64_t)addr & ~0x3FULL;
    uint64_t end = ((uint64_t)addr + size + 63) & ~0x3FULL;
    while (va < end) {
        asm volatile("dc civac, %0" :: "r"(va));
        va += cache_line_size;
    }
    asm volatile("dsb sy");
}
```

### 在 Linux 用户态执行

```bash
# 编译
gcc -o cache_test cache_test.c

# 运行
./cache_test

# 如果需要内核态权限（某些 dc 指令需要 EL1）
# 写内核模块或在 QEMU 裸金属上跑
```

## HFT 关联

实验 15-1 的 CTR_EL0 寄存器在 HFT 中用于动态获取 cache line 大小——代码不应硬编码 64 字节，而应从 CTR_EL0 读取，确保在不同平台上正确对齐。实验 15-2 的 cache 维护指令在 HFT DMA 场景中必需。理解 `dc cvac`（Clean）和 `dc civac`（Flush）的区别可以避免 DMA 数据不一致 bug。

## 自测题

1. **如何用 CTR_EL0 获取 D-cache line 大小？**

<details>
<summary>答案</summary>

```c
u64 ctr;
asm volatile("mrs %0, CTR_EL0" : "=r"(ctr));
int dminline = ctr & 0xF;        // bit[3:0]
int cache_line_size = 4 << dminline;  // 2^(dminline+2) 字节
```
如 dminline=4 → line size = 4 << 4 = 64 字节。
</details>

2. **`dc cvac` 和 `dc civac` 的区别是什么？分别用于什么场景？**

<details>
<summary>答案</summary>

- `dc cvac`：**Clean**（只写回脏数据，不丢弃 cache 行）→ 用于 DMA 读内存前（内存→设备）
- `dc civac`：**Clean + Invalidate**（写回+丢弃）→ 用于自修改代码或需要确保下次从内存读的场景

cvac 后 cache 行仍在，后续可能命中；civac 后 cache 行被丢弃，下次必定 miss。
</details>

3. **在 Linux 用户态可以直接执行 `dc cvac` 吗？有什么限制？**

<details>
<summary>答案</summary>

在 Linux 用户态（EL0）**可以执行** `dc cvac`（ARMv8 允许 EL0 操作 cache），但只能操作自己进程地址空间的 VA。CTR_EL0 也是 EL0 可读的。但 `dc ivac`（Invalidate）通常需要 EL1 权限。实验在 Linux 用户态跑需要内核模块或用 `/dev/mem` 映射物理内存。
</details>

4. **CTR_EL0 的 DIC 和 IDC 位为 1 分别表示什么？对自修改代码有什么影响？**

<details>
<summary>答案</summary>

- **DIC=1**：I-cache 和 D-cache 独立，不需要 clean D-cache → invalidate I-cache 的步骤
- **IDC=1**：硬件自动维护 I-cache 和 D-cache 的一致性

两者都为 1 时（如 Cortex-A76），自修改代码不需要手动 cache 维护——CPU 写新指令后硬件自动保证 I-cache 看到最新数据。但仍建议加 ISB 确保流水线同步。

旧架构（如 A72）DIC=0/IDC=0，必须手动 clean D-cache → invalidate I-cache。
</details>

## 参考与延伸

- [§15.4 关键概念](04-key-concepts.md) — Clean/Invalidate/Flush 定义
- [§15.5 DMA 与 Cache](05-dma-cache.md) — cache 维护指令在 DMA 中的使用
- [§15.7 易错点](07-pitfalls.md) — 实验中的常见错误
