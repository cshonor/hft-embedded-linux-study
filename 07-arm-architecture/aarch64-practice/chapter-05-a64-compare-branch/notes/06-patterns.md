# 5.6 典型代码模式

> 来源：§5.6 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

比较与跳转的常见代码模式：循环、条件赋值、多路分支。

## 核心要点

### 计数循环
```asm
mov x0, #0          ; i = 0
loop:
    ; ... 循环体 ...
    add x0, x0, #1
    cmp x0, #100
    b.lt loop       ; i < 100
```

### 无分支 max
```asm
cmp x0, x1
csel x2, x0, x1, ge  ; x2 = max(x0, x1)
```

### 空指针检查
```asm
cbz x0, null_error
; ... 使用 x0 指针 ...
```

### 状态标志分发
```asm
tbz x0, #0, state0   ; bit0=0 → state0
tbz x0, #1, state1   ; bit1=0 → state1
; bit0=1 and bit1=1 → state2
```

## HFT 关联

这些模式在 HFT 代码中无处不在：
- 计数循环用于批量处理订单 → 用 `b.lt` 避免边界错误
- 无分支 max/min/abs 是低延迟数值操作的核心 → CSEL
- 空指针检查是防御性编程 → CBZ 一条指令
- 状态标志分发用于订单状态机 → TBZ 按位测试

## 自测题

1. 写一个循环，遍历 x0 个元素的数组（8字节元素），将每个元素累加到 x1。
<details><summary>答案</summary>
```asm
mov x1, #0
mov x2, #0          ; index
loop:
    cmp x2, x0
    b.ge done
    ldr x3, [x4, x2, lsl #3]  ; x4 = base
    add x1, x1, x3
    add x2, x2, #1
    b loop
done:
```
</details>

2. 用 CSEL 实现绝对值 abs(x0) → x1。
<details><summary>答案</summary>
```asm
cmp x0, #0
cneg x1, x0, mi    ; mi(N=1,负数) → x1 = -x0；否则 x1 = x0
```
CNEG 是 CSEL 的变体，条件为真时取反。
</details>

3. TBZ 相比 AND+B.EQ 有什么优势？
<details><summary>答案</summary>
1. 1 条指令 vs 2 条（AND 设标志 + B.EQ）
2. 不修改 NZCV 标志，不影响后续条件判断
3. 直接编码位号，不需要构造掩码立即数
4. 不需要临时寄存器存 AND 结果
</details>

## 参考与延伸

- 原书 §5.6
- [5.2 CSEL](02-csel.md)
- [5.5 CBZ/TBZ](05-cbz-tbz.md)
