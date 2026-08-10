# 3.7 典型实操模式

> 来源：§3.7 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

memcpy 骨架、函数栈帧、数组索引三种典型 Load-Store 代码模式。

## 核心要点

### memcpy 骨架
```asm
; x0=dst, x1=src, x2=count(bytes)
memcpy:
    cbz x2, done
loop:
    ldr x3, [x1], #8      ; 读源，后变基
    str x3, [x0], #8      ; 写目标，后变基
    subs x2, x2, #8       ; count -= 8
    b.gt loop
done:
    ret
```

### 函数栈帧
```asm
func:
    stp x29, x30, [sp, #-16]!  ; 保存 FP, LR
    mov x29, sp                 ; 设置帧指针
    ; ... 函数体 ...
    ldp x29, x30, [sp], #16    ; 恢复
    ret
```

### 数组索引
```asm
; x0 = base, x1 = index, 取 arr[index] (8字节元素)
ldr x2, [x0, x1, lsl #3]   ; 地址 = base + index*8
```

## HFT 关联

这些模式在 HFT 代码中无处不在：
- memcpy 骨架：市场数据拷贝 → 用后变基减少指令数
- 函数栈帧：热路径函数的调用开销 → 叶子函数省栈帧
- 数组索引：订单簿数组访问 → `lsl #3` 一条指令完成索引计算
- GDB 调试：`info registers`、`x/8gx $sp` 验证内存布局

## 自测题

1. 以下 memcpy 代码有什么问题？
```asm
loop:
    ldr x3, [x1], #8
    str x3, [x0], #8
    subs x2, x2, #8
    b.gt loop
```
<details><summary>答案</summary>
如果 count 不是 8 的倍数，会多读/多写越界字节。需要先处理对齐余数，或者用字节拷贝处理尾部。
</details>

2. `ldr x2, [x0, x1, lsl #3]` 中 `lsl #3` 的作用是什么？
<details><summary>答案</summary>
将索引 x1 左移 3 位（乘以 8），因为数组元素是 8 字节。这样一条指令就能计算 `base + index * 8` 的地址。
</details>

3. 为什么函数栈帧要保存 x29(FP)？不保存行不行？
<details><summary>答案</summary>
保存 FP 用于调试回溯（backtrace）和异常展开。不保存也能运行（编译器 `-fomit-frame-pointer`），但失去调试能力。叶子函数通常不需要保存 FP。
</details>

## 参考与延伸

- 原书 §3.7
- [3.4 STP/LDP](04-stp-ldp.md)
- [3.3 寻址模式](03-addressing-modes.md)
