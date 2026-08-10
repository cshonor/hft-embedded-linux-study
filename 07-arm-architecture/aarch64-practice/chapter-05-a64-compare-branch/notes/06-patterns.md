# 5.6 典型代码模式

> 来源：§5.6 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

比较与跳转指令在实际代码中的常见组合模式：计数循环、条件赋值、多路分支、空指针检查、标志位分发。

## 核心代码模式

### 模式 1：计数循环

```asm
; for (i = 0; i < count; i++) { ... }
MOV x0, #0              ; i = 0
loop:
    ; ... 循环体 ...
    ADD x0, x0, #1       ; i++
    CMP x0, x1           ; 比较 i < count（x1 = count）
    B.LT loop            ; 有符号小于 → 继续循环
```

**优化版**（倒计数，省一条 CMP）：
```asm
; 倒计数到 0：不需要每次 CMP
MOV x0, x1              ; i = count
loop:
    ; ... 循环体 ...
    SUBS x0, x0, #1      ; i--，同时设标志
    B.NE loop            ; i != 0 → 继续（Z=0 表示非零）
```

> SUBS 自带标志设置，省去 CMP。B.NE 判断 Z=0（结果非零）。这是 ARM 循环的标准优化。

### 模式 2：无分支 max / min / abs

```asm
; max(x0, x1) → x2
CMP x0, x1
CSEL x2, x0, x1, GE     ; x0 >= x1 ? x0 : x1

; min(x0, x1) → x2
CMP x0, x1
CSEL x2, x1, x0, GE     ; x0 >= x1 ? x1 : x0（注意源顺序调换）

; abs(x0) → x1
CMP x0, #0
CNEG x1, x0, MI         ; x0 < 0 ? -x0 : x0

; clamp(x0, lo, hi) → x0（x1=lo, x2=hi）
CMP x0, x1
CSEL x0, x1, x0, LT     ; if (x0 < lo) x0 = lo
CMP x0, x2
CSEL x0, x2, x0, GT     ; if (x0 > hi) x0 = hi
```

### 模式 3：空指针检查

```asm
; if (ptr == NULL) goto error;
func:
    CBZ x0, null_error      ; 1 条指令完成判空+跳转
    LDR x1, [x0]            ; 安全解引用
    ...

null_error:
    MOV x0, #-1             ; 返回错误码
    RET
```

### 模式 4：状态标志分发

```asm
; 根据 x0 的 bit[1:0] 分发到 4 个处理函数
TBZ x0, #0, state_0_or_2    ; bit0=0 → 0或2
TBNZ x0, #1, state_3         ; bit0=1, bit1=1 → 3
B state_1                     ; bit0=1, bit1=0 → 1
state_0_or_2:
TBNZ x0, #1, state_2         ; bit0=0, bit1=1 → 2
; bit0=0, bit1=0 → 0
state_0:
    ; ...
```

**更优雅的跳转表方式**：
```asm
; 用基址+偏移跳转（适合大量分支）
AND x1, x0, #3              ; x1 = x0 & 3（取低2位）
ADR x2, jump_table
LDR x2, [x2, x1, LSL #3]    ; x2 = jump_table[x1]（8字节指针）
BR x2                        ; 间接跳转

jump_table:
    .quad handler_0
    .quad handler_1
    .quad handler_2
    .quad handler_3
```

### 模式 5：条件计数

```asm
; 统计数组中大于阈值的元素个数
; x0 = base, x1 = count, x2 = threshold → x3 = result
MOV x3, #0                   ; result = 0
MOV x4, #0                   ; i = 0
loop:
    CMP x4, x1
    B.GE done
    LDR x5, [x0, x4, LSL #3] ; arr[i]
    CMP x5, x2
    CINC x3, x3, GT          ; if (arr[i] > threshold) result++（CSINC 别名）
    ADD x4, x4, #1
    B loop
done:
```

> CINC 是 CSINC 的别名：条件成立时 x3 = x3 + 1，否则 x3 = x3。

### 模式 6：提前退出

```asm
; 在数组中查找目标值，找到返回索引，未找到返回 -1
; x0 = base, x1 = count, x2 = target → x0 = index or -1
MOV x3, #0                   ; i = 0
loop:
    CMP x3, x1
    B.GE not_found
    LDR x4, [x0, x3, LSL #3]
    CMP x4, x2
    B.EQ found               ; arr[i] == target → 找到
    ADD x3, x3, #1
    B loop
found:
    MOV x0, x3               ; 返回索引
    RET
not_found:
    MOV x0, #-1              ; 返回 -1
    RET
```

### 模式 7：尾调用优化

```asm
; 尾调用：函数最后一步是调用另一个函数
; 用 B 替代 BL+RET，省一层栈帧
wrapper:
    STP x29, x30, [sp, #-16]!
    ; ... 前处理 ...
    LDP x29, x30, [sp], #16   ; 恢复栈帧（因为后面是尾调用，不返回到这里）
    B inner_func              ; 尾调用：直接跳转，inner_func 的 RET 会回到 wrapper 的调用者

; 对比：非尾调用
wrapper2:
    STP x29, x30, [sp, #-16]!
    BL inner_func             ; 调用 → 返回到下一条指令
    ; ... 后处理 ...
    LDP x29, x30, [sp], #16
    RET
```

## 模式总结表

| 模式 | 核心指令 | HFT 价值 |
|------|----------|----------|
| 计数循环 | SUBS+B.NE | 省 CMP，减1周期 |
| 无分支 max/min | CMP+CSEL | 消除预测失败 |
| 无分支 abs | CMP+CNEG | 消除预测失败 |
| 空指针检查 | CBZ | 1条指令 |
| 标志位测试 | TBZ/TBNZ | 不需掩码 |
| 条件计数 | CINC/CINV | 无分支自增 |
| 尾调用 | B（替代BL+RET）| 省一层栈帧 |

## HFT 关联

这些模式在 HFT 代码中无处不在：
- 计数循环用于批量处理订单 → 用 `SUBS+B.NE` 避免额外 CMP
- 无分支 max/min/abs 是低延迟数值操作的核心 → CSEL/CNEG
- 空指针检查是防御性编程 → CBZ 一条指令
- 状态标志分发用于订单状态机 → TBZ 按位测试
- 条件计数用于统计 → CINC 无分支自增
- 尾调用优化减少栈帧开销 → B 替代 BL+RET

```asm
; HFT 综合示例：限价单匹配引擎核心循环
; x0 = order_book, x1 = num_orders, x2 = best_bid
match_loop:
    CBZ x1, done                ; 无订单 → 退出
    LDR x3, [x0], #8            ; 加载订单价格，后变基
    CMP x3, x2                  ; 比较 order_price vs best_bid
    B.GE match                  ; 价格 >= best_bid → 成交
    SUBS x1, x1, #1             ; count--
    B.NE match_loop             ; 还有订单 → 继续
    B done
match:
    ; ... 执行成交 ...
    SUBS x1, x1, #1
    B.NE match_loop
done:
```

## 自测题

1. 写一个循环，遍历 x0 个元素的数组（8字节元素），将每个元素累加到 x1。
<details><summary>答案</summary>
```asm
MOV x1, #0
MOV x2, #0          ; index
loop:
    CMP x2, x0
    B.GE done
    LDR x3, [x4, x2, LSL #3]  ; x4 = base
    ADD x1, x1, x3
    ADD x2, x2, #1
    B loop
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

3. 为什么倒计数循环 `SUBS x0, x0, #1; B.NE loop` 比正计数 `ADD+CMP+B.LT` 更优？
<details><summary>答案</summary>
倒计数用 SUBS 一条指令同时完成减法和标志设置（省去 CMP），循环体少一条指令。B.NE 判断 Z=0（结果非零），等价于 x0 != 0 → 继续循环。每个迭代少 1 条指令 = 少 1 cycle，在紧密循环中累积效果显著。
</details>

4. 用 CINC 实现"如果 x0 > x1 则 x2 自增 1"。
<details><summary>答案</summary>
```asm
CMP x0, x1
CINC x2, x2, GT    ; GT(有符号大于) → x2 = x2 + 1；否则 x2 = x2
```
CINC 是 CSINC 的别名：条件成立时 `Xd = Xn + 1`，不成立时 `Xd = Xn`。无分支实现条件自增。
</details>

5. 写一个 clamp 函数：将 x0 限制在 [x1, x2] 范围内。
<details><summary>答案</summary>
```asm
; clamp(x0, lo=x1, hi=x2) → x0
CMP x0, x1
CSEL x0, x1, x0, LT     ; if (x0 < lo) x0 = lo
CMP x0, x2
CSEL x0, x2, x0, GT     ; if (x0 > hi) x0 = hi
; x0 已被 clamp
```
两条 CSEL 实现无分支 clamp，恒定 2 cycle，无分支预测失败风险。
</details>

## 参考与延伸

- 原书 §5.6
- [5.2 CSEL](02-csel.md)
- [5.5 CBZ/TBZ](05-cbz-tbz.md)
- [5.3 跳转指令](03-branch.md)
