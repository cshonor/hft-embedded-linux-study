# 5.2 条件选择指令 CSEL / CSET / CSINC / CSINV / CNEG

> 来源：§5.2 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

CSEL 系列条件选择指令，实现**无分支**的条件赋值——不跳转，根据 NZCV 标志选择不同的源值写入目标寄存器。

## 核心要点

### 指令全览

| 指令 | 作用 | 伪代码 | 说明 |
|------|------|--------|------|
| CSEL Xd, Xn, Xm, cond | 条件选择 | Xd = cond ? Xn : Xm | 最常用 |
| CSET Xd, cond | 条件置 1 | Xd = cond ? 1 : 0 | 标志转布尔 |
| CSINC Xd, Xn, Xm, cond | 选择+自增 | Xd = cond ? Xn : Xm+1 | 条件假时+1 |
| CSINV Xd, Xn, Xm, cond | 选择+取反 | Xd = cond ? Xn : ~Xm | 条件假时按位取反 |
| CNEG Xd, Xn, cond | 条件取反 | Xd = cond ? -Xn : Xn | 条件真时取负 |

> **CSET 本质**：`CSET Xd, cond` ≡ `CSINC Xd, XZR, XZR, cond`（条件为真选 XZR=0... 不对，CSET 是条件为真时输出 1）。实际上 CSET 的编码是 `CSINC Xd, XZR, XZR, invert(cond)`，即条件为假时取 XZR+1=1，条件为真时取 XZR=0... 也不对。正确关系：`CSET Xd, cond` = `CSINC Xd, XZR, XZR, invert(cond)`，当 cond 成立时选 XZR=0？不。实际是：cond 成立 → Xd=1，cond 不成立 → Xd=0。编码上是 `CSINC Xd, XZR, XZR, !cond`：!cond 成立选 XZR=0，!cond 不成立选 XZR+1=1。所以 cond 成立时 Xd=1。

### CSEL 详解

```asm
; max(x0, x1) → x2
cmp x0, x1
csel x2, x0, x1, ge   ; x0 >= x1 ? x0 : x1

; min(x0, x1) → x2
cmp x0, x1
csel x2, x1, x0, ge   ; x0 >= x1 ? x1 : x0（注意源操作数顺序调换）
```

参数顺序记忆：`CSEL 目的, 条件真时选的源, 条件假时选的源, 条件`

### CSET 详解

```asm
; 将比较结果转为 0/1 布尔值
cmp x0, x1
cset x2, eq           ; x2 = (x0 == x1) ? 1 : 0
cset x2, ne           ; x2 = (x0 != x1) ? 1 : 0
cset x2, ge           ; x2 = (x0 >= x1) ? 1 : 0

; 配合 AND/OR 做条件逻辑
cmp x0, #0
cset x1, eq           ; x1 = (x0 == 0)
cmp x2, #0
cset x3, eq           ; x3 = (x2 == 0)
and x4, x1, x3        ; x4 = (x0==0) && (x2==0)
```

### CSINC / CSINV / CNEG

```asm
; CSINC：条件假时自增
csinc x0, x1, x2, eq  ; eq → x0=x1；ne → x0=x2+1

; CSINV：条件假时取反
csinv x0, x1, x2, eq  ; eq → x0=x1；ne → x0=~x2（按位取反）

; CNEG：条件真时取负
cmp x0, #0
cneg x1, x0, mi       ; mi(负数) → x1 = -x0；否则 x1 = x0
; 这就是 abs(x0) 的实现！
```

### CNEG 与 CSNEG 的关系

| 指令 | 条件真时 | 条件假时 |
|------|----------|----------|
| CNEG Xd, Xn, cond | Xd = -Xn | Xd = Xn |
| CSINV Xd, Xn, Xm, cond | Xd = Xn | Xd = ~Xm |

CNEG 本质是 `CSINV Xd, Xn, Xn, invert(cond)` 的别名：cond 成立时输出 Xn，不成立时输出 ~Xn。而 -Xn = ~Xn + 1？不完全等价——实际上 CNEG 的编码是条件成立时做 SUB Xd, XZR, Xn（即 -Xn），条件不成立时直接传 Xn。

### 条件列表

CSEL 系列支持所有 14 种条件后缀（EQ/NE/CS/CC/MI/PL/VS/VC/HI/LS/GE/LT/GT/LE）。

## 为什么 CSEL 比分支快

```
分支方式（if-else 编译结果）：
  cmp x0, x1
  b.lt else_branch       ; 预测失败时 ~20 cycles 流水线冲刷
  mov x2, x0             ; if 分支
  b done
else_branch:
  mov x2, x1             ; else 分支
done:

CSEL 方式：
  cmp x0, x1
  csel x2, x0, x1, ge    ; 始终 1 cycle，无跳转，无预测
```

| 特性 | 分支 (B.cond) | CSEL |
|------|---------------|------|
| 延迟（预测正确） | ~1 cycle | ~1 cycle |
| 延迟（预测失败） | ~20 cycles | ~1 cycle |
| 代码体积 | 3-4 条指令 | 2 条指令 |
| 延迟可预测性 | 差（依赖预测） | 好（恒定） |
| 适用场景 | 复杂控制流 | 简单条件赋值 |

## 与 C 的对照

```c
// C 代码
int max_val = (a >= b) ? a : b;
int abs_val = (a < 0) ? -a : a;
int is_zero = (a == 0) ? 1 : 0;
```

```asm
// 编译器自动生成
cmp w0, w1
csel w2, w0, w1, ge    ; max_val

cmp w0, #0
cneg w3, w0, mi         ; abs_val

cmp w0, #0
cset w4, eq             ; is_zero
```

## 常见错误

1. **CSEL 参数顺序搞反**：`CSEL Xd, Xn, Xm, cond` —— 条件为真选 Xn，条件为假选 Xm。记不住就记住"真前假后"。
2. **忘记先 CMP**：CSEL 读的是 NZCV 标志，如果前一条指令不是 CMP/SUBS/ADDS，标志可能是旧值。
3. **以为 CSEL 能做复杂判断**：CSEL 只能做二选一，不能替代 if-else if-else 链。复杂逻辑仍需分支。

## HFT 关联

无分支代码是 HFT 低延迟的核心技术：
- `csel` 实现 max/min/abs 无分支 → 消除分支预测失败（~20 cycles penalty）
- 条件赋值避免 if-else 产生的跳转 → 流水线不中断
- 编译器会自动把简单三元运算符转换为 CSEL
- 在热路径中手动用内联汇编确保 CSEL 生成
- 延迟可预测性比平均延迟更重要——CSEL 恒定 1 cycle

```asm
; HFT 典型：无分支价格 clamp（限制在 [lo, hi] 区间）
; x0 = price, x1 = lo, x2 = hi, 结果 → x0
cmp x0, x1
csel x0, x1, x0, lt     ; if (price < lo) price = lo
cmp x0, x2
csel x0, x2, x0, gt     ; if (price > hi) price = hi
```

## 自测题

1. 用 CSEL 实现 max(x0, x1) 到 x2。
<details><summary>答案</summary>
```asm
cmp x0, x1
csel x2, x0, x1, ge   ; x0 >= x1 ? x0 : x1
```
</details>

2. CSET x0, eq 的作用是什么？
<details><summary>答案</summary>
如果 EQ 条件成立（Z=1）则 x0=1，否则 x0=0。等价于将条件标志转换为布尔值。常用于将 CMP 结果转为 0/1 值。
</details>

3. 为什么 CSEL 比分支更快？
<details><summary>答案</summary>
分支有预测失败风险：预测正确时 ~1 cycle，预测失败时 ~20 cycles（流水线冲刷）。CSEL 无条件执行，始终 ~1 cycle，延迟可预测。在 HFT 中延迟可预测性比平均延迟更重要。
</details>

4. 用 CNEG 实现 abs(x0) → x1，并解释原理。
<details><summary>答案</summary>
```asm
cmp x0, #0
cneg x1, x0, mi    ; mi(N=1,负数) → x1 = -x0；否则 x1 = x0
```
CNEG 在条件成立时对源操作数取负，不成立时原样传递。MI 条件 = N=1 = 结果为负，所以负数取负变正，正数不变 → 绝对值。
</details>

5. `csinv x0, x1, x2, eq` 执行后（EQ 不成立），x0 是什么？
<details><summary>答案</summary>
EQ 不成立时，x0 = ~x2（x2 的按位取反）。CSINV 的规则：条件成立选 Xn，条件不成立选 ~Xm。这里条件是 EQ，不成立时取 Xm=x2 的按位取反。
</details>

## 参考与延伸

- 原书 §5.2
- [4.2 NZCV](../../chapter-04-a64-arithmetic-shift/notes/02-nzcv.md)
- [4.6 典型例子](../../chapter-04-a64-arithmetic-shift/notes/06-examples.md)
- [5.6 典型代码模式](06-patterns.md)
