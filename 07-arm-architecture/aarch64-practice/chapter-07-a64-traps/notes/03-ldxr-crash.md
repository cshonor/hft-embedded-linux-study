# 7.3 LDXR 导致死机

> 来源：§7.3 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

在不可独占监视的内存区域执行 LDXR 会导致异常或死机——这是内核开发中的高频陷阱。

## 核心要点

### LDXR 的前提条件

LDXR/STXR 依赖**独占监视器（Exclusive Monitor）**硬件。独占监视器只对特定内存类型有效：

| 内存类型 | 独占监视 | LDXR 行为 |
|----------|----------|-----------|
| Normal Cacheable | ✓ 支持 | 正常工作 |
| Normal Non-cacheable | 部分支持 | 取决于实现 |
| Device | ✗ 不支持 | **未定义/异常** |
| 未开 MMU 的物理地址 | 不确定 | 可能失败 |

### 内存属性与 MAIR_ELx

```
MAIR_EL1（Memory Attribute Indirection Register）定义了内存属性：

  ATTR0: Device-nGnRnE    → Device 内存（最严格）
  ATTR1: Normal-NC         → Normal 不可缓存
  ATTR2: Normal-WT         → Normal 写穿透
  ATTR3: Normal-WB-RA-WA   → Normal 写回（最常用，支持独占监视）

页表项中的 ATTR 位选择使用哪个属性。
→ Normal-WB 属性的内存支持 LDXR/STXR
→ Device 属性的内存不支持
```

### 典型死机场景

```asm
; 场景1：在 MMIO 寄存器上用 LDXR（Device 内存）
LDR X0, =0x09000000        ; UART 寄存器地址（Device 内存）
LDXR X1, [X0]              ; ❌ Device 不可独占监视 → 死机！

; 场景2：在 DMA buffer 上用自旋锁
; 如果 DMA buffer 被映射为 Device 内存 → LDXR 异常

; 场景3：MMU 未开时用 LDXR
; 物理地址可能没有独占监视器 → LDXR 行为未定义
```

### 正确做法

```asm
; 1. MMIO 寄存器用普通 LDR/STR（不需要原子性）
LDR X0, =0x09000000        ; UART base
LDR W1, [X0]               ; 读 UART 状态
STR W2, [X0]               ; 写 UART 数据

; 2. 原子操作只在 Normal Cacheable 内存上使用
; 确保页表将该区域标记为 Normal-WB
ADR X0, shared_counter     ; Normal 内存地址
retry:
    LDXR W1, [X0]
    ADD  W1, W1, #1
    STXR W2, W1, [X0]
    CBNZ W2, retry

; 3. 外设的原子位操作用专用寄存器
; 如 GIC 的 GICD_ISENABLER（Set-Enable）和 GICD_ICENABLER（Clear-Enable）
; 写 1 到对应位即设置/清除，硬件保证原子性
LDR X0, =GICD_BASE
MOV W1, #(1 << irq_num)
STR W1, [X0, #GICD_ISENABLER_OFFSET]  ; 原子启用中断
```

### 独占监视器的限制

```
全局独占监视器（Global Exclusive Monitor）：
  - 跟踪每个核的 LDXR/STXR 操作
  - LDXR 标记地址为"独占"
  - 其他核写该地址 → 清除独占标记
  - STXR 检查标记是否仍在 → 成功/失败

本地独占监视器（Local Exclusive Monitor）：
  - 每个核内部的监视器
  - 管理同一核内的 LDXR/STXR
  - 可以被 CLREX 指令清除

限制：
  - 独占监视器有容量限制（通常跟踪有限数量的地址）
  - 超过限制 → 旧的标记被丢弃 → STXR 失败
  - 某些实现有超时机制 → LDXR 后太久 STXR 会失败
```

### QEMU vs 真实硬件

```
QEMU：
  - 独占监视器实现不完整
  - LDXR/STXR 总是成功（不模拟竞争失败）
  - 在 Device 内存上 LDXR 不报错（宽松）
  → QEMU 测试通过 ≠ 真实硬件通过

真实硬件（如 Raspberry Pi）：
  - 完整的独占监视器
  - 竞争时 STXR 会失败
  - Device 内存上 LDXR 触发异常
  → 必须在真实硬件上验证原子操作
```

## 与 C 的对照

```c
// C11 原子操作在 Device 内存上也是未定义行为
#include <stdatomic.h>

// 错误：在 MMIO 地址上用原子操作
volatile uint32_t *uart = (void *)0x09000000;
atomic_fetch_add(uart, 1);  // → LDXR/STXR → Device 内存 → 死机！

// 正确：MMIO 用普通读写
*uart = value;  // → STR → 安全

// 正确：原子操作只在 Normal 内存上
atomic_int counter;
atomic_fetch_add(&counter, 1);  // → LDXR/STXR → Normal 内存 → 安全
```

## 常见错误

1. **Device 内存上用自旋锁**：自旋锁底层用 LDXR/STXR，在 Device 内存上 → 死机。
2. **MMU 未开就用原子操作**：物理地址可能没有独占监视器。
3. **只测 QEMU 不测硬件**：QEMU 不完整模拟独占监视器，掩盖 bug。

## HFT 关联

LDXR 死机是内核开发的高频 bug：
- 误在 Device 内存（MMIO）上用自旋锁 → 系统挂死
- DMA buffer 如果标记为 Device → 无锁操作失败
- HFT 交易引擎的自旋锁只在 Normal 内存上使用
- 正确的内存属性设置（MAIR_ELx）是前提

```asm
; HFT：确保共享数据在 Normal Cacheable 内存
; 页表设置时，数据区用 Normal-WB 属性
; 自旋锁/原子操作只在 Normal-WB 区域使用

; HFT 反模式：在 MMIO 映射区放共享数据
LDR X0, =MMIO_REGION      ; Device 内存
retry:
    LDXR W1, [X0]          ; ❌ 死机

; HFT 正模式：共享数据在 Normal 内存
ADR X0, shared_data        ; Normal Cacheable
retry:
    LDXR W1, [X0]          ; ✓ 安全
    ADD  W1, W1, #1
    STXR W2, W1, [X0]
    CBNZ W2, retry
```

## 自测题

1. 为什么 Device 内存上不能使用 LDXR？
<details><summary>答案</summary>
Device 内存属性禁止缓存和预取，也不支持独占监视。LDXR 依赖独占监视器标记地址，Device 内存上监视器无法工作，LDXR 行为未定义，可能返回错误数据或触发异常。
</details>

2. MMIO 寄存器需要原子访问吗？用什么指令？
<details><summary>答案</summary>
MMIO 寄存器通常不需要 read-modify-write 的原子操作——每次读写整个寄存器即可。用普通 LDR/STR。如果必须原子修改特定位，用外设提供的 set/clear 寄存器（如 GIC 的 GICD_ISENABLER），而非 LDXR/STXR。
</details>

3. QEMU 上 LDXR 总是成功，这有什么风险？
<details><summary>答案</summary>
QEMU 的某些配置不完整实现独占监视器，LDXR/STXR 总是成功（不模拟竞争失败）。在 QEMU 上测试通过的无锁代码，在真实硬件上可能因为 STXR 失败导致 bug。必须真实硬件或多核 QEMU 配置下充分测试。
</details>

4. 如何确保 LDXR/STXR 正常工作？
<details><summary>答案</summary>
1. 确保目标地址在 Normal Cacheable 内存区域（页表属性正确）
2. 确保地址对齐（至少自然对齐）
3. MMU 已开启且页表正确配置
4. STXR 后检查返回值并循环重试
5. 在真实硬件上测试（不只依赖 QEMU）
</details>

5. GIC 中断控制器的 GICD_ISENABLER 寄存器如何原子启用一个中断？
<details><summary>答案</summary>
```asm
LDR X0, =GICD_BASE
MOV W1, #(1 << irq_num)
STR W1, [X0, #GICD_ISENABLER_OFFSET]
```
GICD_ISENABLER 是"写入即设置"寄存器——写 1 到第 N 位即启用第 N 号中断，写 0 无效。硬件保证整个写入是原子的，不需要 LDXR/STXR。这是外设为原子操作提供的专用机制。对应的 GICD_ICENABLER 是"写入即清除"。
</details>

## 参考与延伸

- 原书 §7.3
- [6.4 LDXR/STXR 预览](../../chapter-06-a64-other-instructions/notes/04-ldxr-stxr-preview.md)
- [Ch14 内存属性与 MAIR](../../chapter-14-memory-management/notes/section-0-本章完整概述.md)
