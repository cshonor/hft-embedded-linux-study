# §22.8 易错点清单

> **来源：** [Ch22 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

NEON/FP 使用中的常见错误：异常中未保存 FP 寄存器、FPEN 未开启、通道数不匹配、LD3 对齐要求、浮点精度问题。

## 核心要点

### 易错点清单

| # | 错误 | 后果 | 修复 |
|---|------|------|------|
| 1 | NEON 寄存器在异常中没保存 | 中断处理破坏用户态浮点状态 | 保存/恢复 V0-V31 |
| 2 | FPEN 没开 | FP/NEON 指令触发 trap | CPACR_EL1.FPEN = 0b11 |
| 3 | 通道数不匹配 | 汇编报错 | 统一后缀 (.4s/.8h) |
| 4 | LD3 对齐 | 对齐异常 | 地址对齐到通道宽度 |
| 5 | float32 精度不够 | PnL 计算累积误差 | 用 double 或定点 |
| 6 | 编译器自动向量化失败 | 无加速效果 | 用 intrinsics 或 asm |

### 陷阱 1：中断中使用 NEON

```c
// 错误：中断处理中用了 NEON 指令
void irq_handler(void) {
    float32x4_t data = vld1q_f32(buffer);  // 用了 V0
    // ... 处理 ...
    // V0 的原始值被破坏！被中断的代码的浮点状态丢失
}

// 正确：Linux 内核中用 kernel_neon_begin/end 保护
void kernel_neon_safe_handler(void) {
    kernel_neon_begin();    // 保存当前 FP 上下文
    float32x4_t data = vld1q_f32(buffer);
    // ... NEON 处理 ...
    kernel_neon_end();      // 恢复 FP 上下文
}

// 裸机：异常入口手动保存
irq_handler:
    // 保存所有 V 寄存器
    sub sp, sp, #512
    stp q0, q1, [sp, #0]
    stp q2, q3, [sp, #32]
    // ... 保存 q4-q31 ...
    // NEON 处理
    // 恢复所有 V 寄存器
    ldp q0, q1, [sp, #0]
    // ...
    add sp, sp, #512
    eret
```

### 陷阱 2：FPEN 未开

```asm
; 裸机启动时必须配置
MRS X0, CPACR_EL1
ORR X0, X0, #(0b11 << 20)  ; FPEN = 0b11 (EL0+EL1 允许)
MSR CPACR_EL1, X0
ISB                          ; 同步

; 验证
MRS X0, CPACR_EL1
AND X1, X0, #(0b11 << 20)
CBZ X1, fp_not_enabled       ; 如果 FPEN=00 则跳转
```

### 陷阱 3：通道不匹配

```asm
ADD V0.4s, V1.8h, V2.4s   ; 错误！.4s 和 .8h 不能混用
ADD V0.4s, V1.4s, V2.4s   ; 正确：统一 .4s

; 通道数和宽度都必须一致
; .4s = 4×32bit, .8h = 8×16bit
; 通道数不同(4 vs 8)→无法对应
```

### 陷阱 4：LD3 对齐要求

```c
// 错误：未对齐的 LD3
void bad_process(uint8_t *data) {
    // data 可能不对齐到 16 字节
    uint8x16x3_t rgb = vld3q_u8(data);  // 可能触发对齐异常
}

// 正确：确保对齐
void good_process(void) {
    // 分配对齐内存
    uint8_t *data = aligned_alloc(16, 48 * 100);
    uint8x16x3_t rgb = vld3q_u8(data);  // 安全
}

// 或者用非对齐版本（性能略低）
// LD1 默认不要求对齐（但对齐时更快）
```

### 陷阱 5：float32 精度问题

```c
// HFT PnL 计算：float32 精度可能不够
float pnl = 0.0f;
for (int i = 0; i < 100000; i++) {
    pnl += price[i] * qty[i];  // float32 累积误差
}
// 100000 次累加后误差可达 ~0.01%

// 修复1: 用 double
double pnl_d = 0.0;
for (int i = 0; i < 100000; i++)
    pnl_d += (double)price[i] * qty[i];

// 修复2: 用定点（int64, 精确无误差）
int64_t pnl_fixed = 0;
for (int i = 0; i < 100000; i++)
    pnl_fixed += (int64_t)(price[i] * SCALE) * qty[i];
// SCALE = 10000 → 4 位小数精度
```

### 陷阱 6：编译器自动向量化失败

```c
// 编译器可能无法自动向量化（有分支）
void bad_auto_vec(float *a, float *b, int n) {
    for (int i = 0; i < n; i++) {
        if (b[i] > 0)  // 分支阻碍向量化
            a[i] = b[i] * 2.0f;
        else
            a[i] = 0;
    }
}

// 修复1: 消除分支（用位运算）
void good_auto_vec(float *a, float *b, int n) {
    for (int i = 0; i < n; i++)
        a[i] = (b[i] > 0) ? b[i] * 2.0f : 0.0f;
    // 编译器可能用 BIF/BIT 无分支实现
}

// 修复2: 显式用 intrinsics
void explicit_neon(float *a, float *b, int n) {
    float32x4_t zero = vdupq_n_f32(0.0f);
    for (int i = 0; i < n; i += 4) {
        float32x4_t bv = vld1q_f32(b + i);
        float32x4_t mask = vcgtq_f32(bv, zero);
        float32x4_t val = vmulq_n_f32(bv, 2.0f);
        float32x4_t result = vbslq_f32(mask, val, zero);
        vst1q_f32(a + i, result);
    }
}
```

### 调试技巧表

| 症状 | 可能原因 | 调试方法 |
|------|---------|---------|
| FP 指令触发异常 | FPEN 未开 | 检查 CPACR_EL1 |
| 中断后浮点结果错 | 中断用了 NEON | 加 kernel_neon_begin |
| 编译报错通道不匹配 | 后缀不一致 | 统一 .4s/.8h |
| 性能无提升 | 自动向量化失败 | 用 intrinsics |
| 结果有微小偏差 | float32 精度 | 改用 double/定点 |
| 对齐异常 | LD 地址未对齐 | 用 aligned_alloc |

## HFT 关联

HFT 系统中如果交易线程被中断打断，而中断处理中使用了 NEON 寄存器（如网卡驱动的 checksum 计算），会导致交易线程的浮点数据损坏。Linux 内核对此有保护：中断中默认不使用 FP，需要用时通过 `kernel_neon_begin()` 保存上下文。但裸机 HFT（如直接在 ARM 上跑无 OS 的 trading engine）需要自己在异常入口保存 V0-V31。

```c
// HFT NEON 安全检查清单
void hft_neon_safety_check() {
    // 1. 确认 NEON 使能
    uint64_t fpcr;
    asm volatile("mrs %0, fpcr" : "=r"(fpcr));
    if (!(fpcr & (1ULL << 24))) {
        printf("WARN: FZ not set, denormals slow\n");
    }

    // 2. 确认 FPEN 开启
    uint64_t cpacr;
    asm volatile("mrs %0, cpacr_el1" : "=r"(cpacr));
    if (!((cpacr >> 20) & 0x3) == 0x3) {
        printf("ERROR: FPEN not enabled!\n");
    }

    // 3. 检查数据对齐
    void *ptr = market_data;
    if ((uintptr_t)ptr & 0xF) {
        printf("WARN: data not 16-byte aligned\n");
    }

    // 4. 检查 float 精度是否足够
    // (业务逻辑验证)

    // 5. objdump 检查生成代码
    // gcc -O3 -S -mfpu=neon code.c
    // 检查是否有多余的 FMOV (register move)
}
```

## 自测题

1. **中断处理中使用 NEON 指令会有什么问题？Linux 怎么解决的？**

<details>
<summary>答案</summary>

**问题**：中断打断用户态代码时，用户态可能正在使用 V0-V31 寄存器。如果中断处理中直接使用 NEON 指令（如 vld1q），会覆盖用户态的浮点/SIMD 数据，中断返回后用户态浮点结果错误。**Linux 解决方案**：内核默认**不使用** FP/NEON 指令。如果内核代码必须用（如 crypto 加密），调用 `kernel_neon_begin()` 保存当前 FP 上下文、`kernel_neon_end()` 恢复。中断处理中通常不用 NEON——需要 SIMD 的中断处理推迟到 softirq/tasklet 中执行。
</details>

2. **裸机环境如何正确使能浮点？不开启会怎样？**

<details>
<summary>答案</summary>

裸机中需要配置 `CPACR_EL1` 的 FPEN 位（bit 21:20）为 `0b11`（EL0 和 EL1 都允许 FP/NEON），然后执行 ISB 同步。不开启会怎样：任何 FP/NEON 指令（FADD、LD1 等）触发**同步异常**（ESR.EC=0x07 "FP/SIMD trap"），跳转到异常向量表。如果异常处理没有正确处理这个 trap，会导致死循环（异常处理本身也可能用到 FP → 再次触发 trap）。裸机初始化代码应在使能 MMU 后立即配置 FPEN。
</details>

3. **`ADD V0.4s, V1.8h, V2.4s` 为什么会报错？**

<details>
<summary>答案</summary>

因为**通道布局不匹配**。`.4s` = 4 通道 × 32 位，`.8h` = 8 通道 × 16 位。ADD 指令要求源操作数和目的操作数的通道布局相同（每个通道对应相加）。`.4s` 和 `.8h` 通道数不同（4 vs 8）、通道宽度不同（32 vs 16），无法对应。必须统一：`ADD V0.4s, V1.4s, V2.4s` 或 `ADD V0.8h, V1.8h, V2.8h`。这是 NEON 汇编的常见编译错误。
</details>

## 参考与延伸

- [§22.1 浮点寄存器](01-fp-registers.md) — FPEN 和 CPACR_EL1
- [§22.6 NEON 内建函数](06-intrinsics.md) — intrinsics 的类型安全避免通道错误
- [Ch11 异常处理](../../chapter-11-exception-handling/notes/section-0-本章完整概述.md) — 异常入口保存寄存器
