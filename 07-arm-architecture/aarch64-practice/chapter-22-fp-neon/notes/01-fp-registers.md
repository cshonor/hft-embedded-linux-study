# §22.1 浮点寄存器

> **来源：** [Ch22 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

ARMv8 提供 32 个 128 位 SIMD/FP 寄存器 V0-V31，可通过不同名称访问不同宽度的低位部分（Q/D/S/H/B），形成层级视角。

## 核心要点

### V 寄存器层级视角

| 视角 | 宽度 | 名称 | 用途 | C 类型 |
|------|------|------|------|--------|
| 128 位 | 全宽 | Vn / Qn | NEON 向量 | int32x4_t / float32x4_t |
| 64 位 | 低半 | Dn | 双精度浮点 | double / int64x2_t |
| 32 位 | 低四分之一 | Sn | 单精度浮点 | float |
| 16 位 | 低八分之一 | Hn | 半精度浮点 | __fp16 |
| 8 位 | 低十六分之一 | Bn | 字节 | — |

```
V0 [127:0]  ← 128 bit (Q0 / V0)
   [63:0]   ← 64 bit  (D0)
   [31:0]   ← 32 bit  (S0)
   [15:0]   ← 16 bit  (H0)
   [7:0]    ← 8 bit   (B0)

高位 [127:64] 在写 D0 时保持不变
高位 [127:32] 在写 S0 时保持不变
```

### 关键规则

- 写 Sn 只修改 [31:0]，高位 [127:32] **保持不变**
- 写 Dn 只修改 [63:0]，高位 [127:64] **保持不变**
- 写 Qn 修改全部 [127:0]
- FP/SIMD 使能由 `CPACR_EL1.FPEN` 控制

### 与 x86 SSE 的对比

| 维度 | ARMv8 NEON | x86 SSE/AVX |
|------|-----------|-------------|
| 寄存器 | V0-V31 (32×128bit) | XMM0-15 (16×128bit) |
| 低位写高位 | 保持不变 | 高位清零 |
| 浮点异常 | FTZ/DN 模式默认 | 严格 IEEE 754 |
| 寄存器保存 | lazy（按需） | eager（立即） |
| 上下文切换开销 | ~50-100ns | ~100-200ns |
| 256 位支持 | 需 SVE（ARMv9） | AVX (YMM0-15) |

### FPCR 控制寄存器

```c
// FPCR (Floating-Point Control Register) 关键位
// bit 24: FZ (Flush-to-Zero) — 非正规数清零
// bit 12: DN (Default-NaN) — NaN 传播规则简化
// bit 11-9: RMode — 舍入模式
//   00 = Round to Nearest (Ties to Even)
//   01 = Round towards Plus Infinity
//   10 = Round towards Minus Infinity
//   11 = Round towards Zero (Truncate)

// 读取/设置 FPCR
static inline uint64_t get_fpcr(void) {
    uint64_t fpcr;
    asm volatile("mrs %0, fpcr" : "=r"(fpcr));
    return fpcr;
}

// HFT 通常默认 FZ=1, DN=1（性能优先）
```

### FPSR 状态寄存器

```c
// FPSR (Floating-Point Status Register)
// bit 4: NZCV flags (同 PSTATE 但独立)
// bit 27: QC (Saturation flag, NEON 用)
// bit 1-7: Cumulative exception flags

// FPSR 的 NZCV 与 PSTATE 的 NZCV 独立
// FP 比较指令 FCMP 设置 FPSR.NZCV
// 条件分支使用 FPSR flags（如 B.EQ 读 FPSR）
```

## HFT 关联

HFT 中的浮点运算（如 PnL 计算、风险指标）需要注意 FP 寄存器的保存开销——Linux 在线程切换时按需保存 FP 寄存器（lazy FP context switch）。

```c
// HFT 浮点性能测量
// FP 寄存器保存/恢复开销
// 32 × 128bit = 512 字节
// 16 条 STP + 16 条 LDP = 32 条指令
// Cortex-A76: ~50-100ns (full save/restore)

// HFT 减少 FP 保存开销的策略
void hft_fp_optimization() {
    // 1. 绑核 → 不切换线程 → 不保存 FP
    cpu_set_t cs; CPU_ZERO(&cs); CPU_SET(2, &cs);
    sched_setaffinity(0, sizeof(cs), &cs);

    // 2. SCHED_FIFO → 减少被抢占
    struct sched_param sp = { .sched_priority = 99 };
    sched_setscheduler(0, SCHED_FIFO, &sp);

    // 3. 中断不使用 FP → 内核不会保存 FP
    // (Linux 默认中断不用 FP，需 kernel_neon_begin)

    // 4. 设置 FZ=1/DN=1 加速浮点
    // (Linux 用户态默认已设)
}

// HFT float vs double 选择
// float32 + .4s → 4 路并行，~1 cycle/FMA
// double + .2d → 2 路并行，~2 cycle/FMA
// HFT 通常用 float32（精度足够，性能翻倍）
```

| HFT FP 场景 | 推荐类型 | 原因 |
|------------|---------|------|
| PnL 计算 | float32 | 4 路并行，精度够 |
| VaR/Greeks | float32 | 矩阵运算，4x 加速 |
| 定单簿价格 | int64 (定点) | 避免浮点精度问题 |
| 风险模型 | float64 | 累积精度要求高 |

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

4. **FPCR 的 FZ 位和 DN 位对 HFT 有什么影响？**

<details>
<summary>答案</summary>

FZ=1（Flush-to-Zero）：非正规数（denormal）被当作零处理，跳过非正规数处理逻辑，浮点运算更快。DN=1（Default-NaN）：任何产生 NaN 的运算返回默认 NaN，跳过 NaN 传播检查。两者都牺牲 IEEE 754 精确性换取性能。HFT 通常接受这种损失——交易系统用 float32 精度足够，FZ/DN 可带来 10-20% 浮点性能提升。如果需要精确的非正规数处理（如某些金融模型），必须 FZ=0。
</details>

## 参考与延伸

- [§22.2 NEON 向量寄存器](02-neon-vectors.md) — V 寄存器的并行通道拆分
- [§22.6 NEON 内建函数](06-intrinsics.md) — C 语言中使用 NEON
