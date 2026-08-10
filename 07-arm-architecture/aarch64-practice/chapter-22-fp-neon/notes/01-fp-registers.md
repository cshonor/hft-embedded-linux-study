# §22.1 浮点寄存器

> **来源：** [Ch22 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

ARMv8 提供 32 个 128 位 SIMD/FP 寄存器 V0-V31，可通过不同名称访问不同宽度的低位部分（Q/D/S/H/B），形成层级视角。

## 核心要点

### V 寄存器层级视角

| 视角 | 宽度 | 名称 | 用途 |
|------|------|------|------|
| 128 位 | 全宽 | Vn / Qn | NEON 向量 |
| 64 位 | 低半 | Dn | 双精度浮点 |
| 32 位 | 低四分之一 | Sn | 单精度浮点 |
| 16 位 | 低八分之一 | Hn | 半精度浮点 |
| 8 位 | 低十六分之一 | Bn | 字节 |

```
V0 [127:0]  ← 128 bit (Q0)
   [63:0]   ← 64 bit  (D0)
   [31:0]   ← 32 bit  (S0)
   [15:0]   ← 16 bit  (H0)
   [7:0]    ← 8 bit   (B0)
```

### 关键规则

- 写 Sn 只修改 [31:0]，高位 [127:32] **保持不变**
- 写 Dn 只修改 [63:0]，高位 [127:64] **保持不变**
- 写 Qn 修改全部 [127:0]
- FP/SIMD 使能由 `CPACR_EL1.FPEN` 控制

## HFT 关联

HFT 中的浮点运算（如 PnL 计算、风险指标）需要注意 FP 寄存器的保存开销——Linux 在线程切换时按需保存 FP 寄存器（lazy FP context switch）。交易线程如果大量使用浮点，每次切换保存/恢复 32×128bit = 512 字节，约 50-100ns。`SCHED_FIFO` 绑核可以避免线程切换，但中断仍可能使用 FP 寄存器（内核不保存 V0-V31 在中断中），所以内核中断处理不应使用浮点。

## 自测题

1. **写 Sn（32位）后，Vn 的高位 [127:32] 会怎样？**

<details>
<summary>答案</summary>

高位 [127:32] **保持不变**。ARMv8 架构规定：写低位视图（Sn/Dn/Hn/Bn）只修改对应的低位部分，高位不受影响。这与 x86 的 SSE 不同（x86 写 XMM 的低位会清零高位）。这个行为在混合使用浮点和 NEON 时可能导致意外——如果先写 V0.4s（NEON，128位全写），再写 S0（浮点，32位），V0 的高 96 位保留 NEON 写的旧值。
</details>

2. **CPACR_EL1.FPEN 的作用是什么？如果没开会怎样？**

<details>
<summary>答案</summary>

`CPACR_EL1.FPEN` 控制浮点/SIMD 指令的使能。如果 FPEN=0（或 trap 配置不允许），执行任何 FP/NEON 指令会触发**同步异常**（Synchronous Trap，ESR.EC=0x07 "FP/SIMD trap"）。裸机环境中必须在使能 MMU 后、使用浮点前配置 CPACR_EL1.FPEN=0b11（EL0 和 EL1 都允许访问）。Linux 默认开启 FPEN。
</details>

3. **为什么说 FP 寄存器保存是上下文切换的"隐藏开销"？**

<details>
<summary>答案</summary>

通用寄存器切换约 20-50ns，但 FP/SIMD 寄存器有 32 个 128 位 = 512 字节，保存/恢复需要 16 条 STP + 16 条 LDP = 32 条指令，约 50-100ns。Linux 采用 **lazy FP context switch**：切换时不立即保存 FP 寄存器，标记 `TIF_FOREIGN_FPSTATE`，直到新线程实际使用 FP 指令时才触发保存/恢复（通过 trap）。但如果两个线程都密集使用 FP，每次切换都要完整保存/恢复。HFT 绑核可避免此开销。
</details>

## 参考与延伸

- [§22.2 NEON 向量寄存器](02-neon-vectors.md) — V 寄存器的并行通道拆分
- [§22.6 NEON 内建函数](06-intrinsics.md) — C 语言中使用 NEON
