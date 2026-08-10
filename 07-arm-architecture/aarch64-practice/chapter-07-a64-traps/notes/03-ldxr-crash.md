# 7.3 LDXR 导致死机

> 来源：§7.3 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

在不可独占监视的内存区域执行 LDXR 会导致异常或死机。

## 核心要点

LDXR 需要独占监视器支持。以下场景 LDXR 会出问题：
- **Device 内存**：不可缓存也不可独占监视 → LDXR 行为未定义
- **未开 MMU**：物理地址可能没有独占监视器 → LDXR 可能失败
- **非对齐地址**：LDXR 要求对齐 → 不对齐触发异常
- **QEMU 某些配置**：独占监视器未实现 → LDXR 总是成功（不真实）

```asm
; Device 内存上执行 LDXR → 可能死机
ldr x0, =0x09000000   ; UART 寄存器（Device）
ldxr x1, [x0]         ; ❌ Device 不可独占监视
```

正确做法：
- 只在 Normal Cacheable 内存上使用 LDXR/STXR
- MMIO 寄存器用普通 LDR/STR（不需要原子性，外设保证）
- 确保地址对齐

## HFT 关联

LDXR 死机是内核开发的高频 bug：
- 误在 Device 内存（MMIO）上用自旋锁 → 系统挂死
- DMA buffer 如果标记为 Device → 无锁操作失败
- HFT 交易引擎的自旋锁只在 Normal 内存上使用
- 正确的内存属性设置（MAIR_ELx）是前提

## 自测题

1. 为什么 Device 内存上不能使用 LDXR？
<detail><summary>答案</summary>
Device 内存属性禁止缓存和预取，也不支持独占监视。LDXR 依赖独占监视器标记地址，Device 内存上监视器无法工作，LDXR 行为未定义，可能返回错误数据或触发异常。
</details>

2. MMIO 寄存器需要原子访问吗？用什么指令？
<detail><summary>答案</summary>
MMIO 寄存器通常不需要 read-modify-write 的原子操作——每次读写整个寄存器即可。用普通 LDR/STR。如果必须原子修改特定位，用外设提供的 set/clear 寄存器（如 GIC 的 GICD_ISENABLER），而非 LDXR/STXR。
</details>

3. QEMU 上 LDXR 总是成功，这有什么风险？
<detail><summary>答案</summary>
QEMU 的某些配置不完整实现独占监视器，LDXR/STXR 总是成功（不模拟竞争失败）。在 QEMU 上测试通过的无锁代码，在真实硬件上可能因为 STXR 失败导致 bug。必须真实硬件或多核 QEMU 配置下充分测试。
</details>

## 参考与延伸

- 原书 §7.3
- [6.4 LDXR/STXR 预览](../../chapter-06-a64-other-instructions/notes/section-0-本章完整概述.md)
- [Ch14 内存属性](../../chapter-14-memory-management/notes/section-0-本章完整概述.md)
