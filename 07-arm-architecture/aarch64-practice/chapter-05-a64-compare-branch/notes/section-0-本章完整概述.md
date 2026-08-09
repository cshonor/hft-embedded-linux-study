# Ch5 完整总结 · 比较指令与跳转指令

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md) · [Pi5 适配](../../PI5-ADAPT.md)

---

## 本章定位

比较指令（CMP/CMN/CSEL）+ 跳转指令（B/BL/BR/RET/CBZ/TBZ）= AArch64 控制流核心。  
实验优先 **QEMU** `-cpu cortex-a76`。

---

## 5.1 比较指令

| 指令 | 等价操作 | 说明 |
|------|----------|------|
| `CMP Xn, Xm` | `SUBS XZR, Xn, Xm` | 比较，结果丢弃，只改 NZCV |
| `CMN Xn, Xm` | `ADDS XZR, Xn, Xm` | Compare Negative，等价加上后再比较 |

> CMP 是减法 → C 标志反映**无符号**大小；V 反映**有符号**溢出。

```asm
cmp x0, x1      ; 比较 x0 和 x1
b.eq equal      ; Z=1 → 相等则跳
b.lt less       ; N≠V → 有符号小于
b.hs ge_or_eq   ; C=1 → 无符号 ≥
```

---

## 5.2 条件选择指令 ⭐

这是 AArch64 的特色——**不跳转，用条件指令选择结果**，避免分支预测失败。

| 指令 | 行为 |
|------|------|
| `CSEL Rd, Rn, Rm, cond` | 条件成立 → `Rd=Rn`；否则 `Rd=Rm` |
| `CSET Rd, cond` | 条件成立 → `Rd=1`；否则 `Rd=0`（等价 `CSINC Rd, XZR, XZR, invert(cond)`） |
| `CSINC Rd, Rn, Rm, cond` | 同 CSEL，但 else 分支 `Rm+1` |
| `CSINV Rd, Rn, Rm, cond` | 同 CSEL，但 else 分支 `~Rm` |
| `CSNEG Rd, Rn, Rm, cond` | 同 CSEL，但 else 分支 `-Rm` |

```asm
; 求绝对值（无分支）
cmp x0, #0
cneg x0, x0, lt    ; 如果 x0<0（有符号），x0 = -x0

; 求最大值（无分支）
cmp x0, x1
csel x0, x0, x1, ge  ; x0>=x1 → 保留 x0，否则取 x1
```

> **CSEL 是 HFT 常用技巧**：无分支代码避免流水线 flush，延迟可预测。

---

## 5.3 跳转指令全览

| 指令 | 行为 | 用途 |
|------|------|------|
| `B label` | 直接跳转 | 循环、条件分支 |
| `B.cond label` | 条件跳转 | if/else |
| `BL label` | 跳转并保存返回地址到 **X30(LR)** | 函数调用 |
| `BR Xn` | 跳到寄存器中的地址 | 函数指针、switch |
| `BLR Xn` | BL + BR（保存 LR + 跳寄存器） | 间接函数调用 |
| `RET {Xn}` | 返回（默认跳 X30） | 函数返回 |
| `ERET` | 异常返回（ELR+SPSR 恢复） | 异常处理返回 |

> `BL` 把返回地址写入 **X30(LR)**，不像 x86 压栈。嵌套调用必须手动保存 LR。

### 嵌套调用：保存 LR

```asm
; func_a 调用 func_b，func_b 又调用 func_c
func_a:
    stp x29, x30, [sp, #-16]!  ; 保存 FP + LR
    bl  func_b
    ldp x29, x30, [sp], #16    ; 恢复
    ret

; 如果 func_b 不再调用其他函数（叶子函数），可以不保存 LR
func_leaf:
    ; 不需要 stp/ldp x30
    ret
```

---

## 5.4 条件后缀速查 ⭐

| 后缀 | 含义 | 条件 |
|------|------|------|
| `EQ` | Equal | Z=1 |
| `NE` | Not Equal | Z=0 |
| `HS` / `CS` | 无符号 ≥ | C=1 |
| `LO` / `CC` | 无符号 < | C=0 |
| `MI` | 负 | N=1 |
| `PL` | 非负 | N=0 |
| `VS` | 有符号溢出 | V=1 |
| `VC` | 无溢出 | V=0 |
| `HI` | 无符号 > | C=1 && Z=0 |
| `LS` | 无符号 ≤ | C=0 \|\| Z=1 |
| `GE` | 有符号 ≥ | N==V |
| `LT` | 有符号 < | N≠V |
| `GT` | 有符号 > | Z=0 && N==V |
| `LE` | 有符号 ≤ | Z=1 \|\| N≠V |

> **记法**：
> - `HS/LO/HS/HI/LS` = 无符号（看 C 和 Z）
> - `GE/LT/GT/LE` = 有符号（看 N、V、Z）
> - `HS` = Higher or Same；`HI` = Higher

---

## 5.5 CBZ / CBNZ / TBZ / TBNZ

比较并跳转——**合并 CMP + B**，更紧凑。

| 指令 | 行为 |
|------|------|
| `CBZ Xn, label` | Xn==0 → 跳 |
| `CBNZ Xn, label` | Xn≠0 → 跳 |
| `TBZ Xn, #bit, label` | Xn 的第 bit 位==0 → 跳 |
| `TBNZ Xn, #bit, label` | Xn 的第 bit 位==1 → 跳 |

```asm
; 空指针检查
cbz x0, null_ptr    ; x0 为 0 → 跳到错误处理

; 位测试
tbnz x0, #7, negative  ; x0 符号位=1 → 负数
```

> CBZ/CBNZ 只能跟 0 比；要和特定值比还是用 CMP + B.cond。

---

## 5.6 典型代码模式

### if-else

```asm
    cmp x0, x1
    b.lt less
    ; greater or equal 分支
    ...
    b done
less:
    ; less 分支
    ...
done:
```

### 循环

```asm
    mov x0, #0        ; i = 0
    mov x1, #100      ; n = 100
loop:
    ; ... loop body ...
    add x0, x0, #1    ; i++
    cmp x0, x1
    b.lo loop         ; i < n (无符号)
```

### switch（用 BR）

```asm
    ; x0 = case index (0-3)
    adr x1, jump_table
    ldr x0, [x1, x0, lsl #3]  ; 8字节指针 × index
    br x0
jump_table:
    .quad case0
    .quad case1
    .quad case2
    .quad case3
```

---

## 5.7 实验要点

| 实验 | 内容 | 平台 |
|------|------|------|
| 5-1 | CMP 和 CMN 指令 | QEMU |
| 5-2 | 条件选择指令（CSEL/CSET） | QEMU |
| 5-3 | 子函数跳转（BL/RET，嵌套调用保存 LR） | QEMU |

---

## 5.8 易错点清单

1. **BL 嵌套不存 LR** → 返回地址被覆盖，返回到错误地址。
2. **CSEL 不是跳转** → 它是条件选择，不改变 PC。
3. **B.cond 看什么标志** → 有符号用 GE/LT/GT/LE（N、V），无符号用 HS/LO/HI/LS（C）。
4. **CBZ 只跟 0 比** → 要和非零值比较用 CMP。
5. **RET 默认跳 X30** → 如果 LR 被覆盖（嵌套 BL），RET 会跳错。

---

## 书中思考题（自测）

1. BL 和 B 的区别？BL 把返回地址存哪？
2. CSEL 指令的作用？为什么说它适合 HFT？
3. 嵌套调用时为什么要保存 X30？叶子函数需要吗？
4. 有符号大于跳转用哪个条件后缀？无符号大于呢？
5. CBZ 和 CMP+B.EQ 什么关系？

**参考答案：**

1. BL 把返回地址存 **X30(LR)**；B 不存。  
2. 条件选择——不跳转，直接选结果，**避免分支预测失败**，延迟可预测。  
3. 嵌套 BL 会覆盖 X30，所以必须 `stp x29,x30,[sp,#-16]!` 保存；**叶子函数不需要**。  
4. 有符号大于 = **GT**（Z=0 && N==V）；无符号大于 = **HI**（C=1 && Z=0）。  
5. CBZ = CMP + B.EQ 的**合并形式**，更紧凑，但不改变 NZCV 标志。

---

上一章 [Ch4 算术/移位](../../chapter-04-a64-arithmetic-shift/) · 下一章 [Ch6 杂项指令](../../chapter-06-a64-other-instructions/) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md)
