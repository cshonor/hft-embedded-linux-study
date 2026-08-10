# 5.7 实验要点

> 来源：§5.7 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

通过 QEMU + GDB 实验，动手验证比较与分支指令的行为，理解 CSEL 无分支的优势和条件后缀的正确选择。

## 实验列表

| 实验 | 内容 | 平台 | 重点 |
|------|------|------|------|
| 5-1 | 条件跳转与循环 | QEMU | CMP + B.cond 的 NZCV 变化 |
| 5-2 | CSEL 实现无分支 | QEMU | 对比 if-else vs CSEL 反汇编 |
| 5-3 | CBZ/TBZ 位测试 | QEMU | 1 条指令替代 2 条 |
| 5-4 | 条件后缀综合 | QEMU | 有符号 vs 无符号陷阱 |
| 5-5 | 尾调用优化 | QEMU | B vs BL+RET 栈帧对比 |

## 实验环境准备

```bash
# QEMU 启动裸机程序
qemu-system-aarch64 -M raspi3b -kernel my_program.elf -serial stdio

# GDB 连接
aarch64-linux-gnu-gdb my_program.elf
(gdb) target remote localhost:1234
```

## 实验 5-1：条件跳转与循环

**目标**：观察 CMP 后 NZCV 的变化，理解 B.cond 如何根据标志跳转。

**步骤**：

1. 编写计数循环，用 GDB 单步执行
2. 每步检查 `p/x $pstate` 中的 NZCV 位

```asm
; exp5_1.s
.global _start
_start:
    MOV x0, #0              ; i = 0
    MOV x1, #5              ; count = 5
loop:
    ADD x0, x0, #1           ; i++
    CMP x0, x1               ; i < count ?
    B.LT loop                ; 继续循环
    ; x0 = 5 时退出
done:
    B done
```

**GDB 操作**：
```
(gdb) b loop
(gdb) c
(gdb) p/x $x0               ; 查看 i
(gdb) p/x $pstate           ; 查看 NZCV（在 pstate 的 31-28 位）
; N=bit31, Z=bit30, C=bit29, V=bit28
; 例：pstate=0x60000000 → Z=1,C=1 → 表示结果为零且无借位
```

**验证点**：
- i=1, count=5: 1-5=-4 → N=1, Z=0, C=0, V=0 → LT(N≠V=1≠0)成立 → 继续
- i=5, count=5: 5-5=0 → N=0, Z=1, C=1, V=0 → LT(N≠V=0≠0)不成立 → 退出

## 实验 5-2：CSEL 无分支

**目标**：对比 if-else 和 CSEL 编译后的差异。

**步骤**：

1. 写 C 代码用 `gcc -O2 -S` 编译
2. 观察是否生成 CSEL 指令

```c
// exp5_2.c
int max_val(int a, int b) {
    return (a >= b) ? a : b;
}
int abs_val(int a) {
    return (a < 0) ? -a : a;
}
```

```bash
aarch64-linux-gnu-gcc -O2 -S exp5_2.c -o exp5_2.s
```

**预期输出**：
```asm
max_val:
    CMP w0, w1
    CSEL w0, w0, w1, GE    ; 编译器自动生成 CSEL
    RET

abs_val:
    CMP w0, #0
    CNEG w0, w0, MI         ; 编译器自动生成 CNEG
    RET
```

**验证点**：
- `-O0` 可能不生成 CSEL（用分支实现）
- `-O2` 编译器会自动优化为 CSEL
- 用 GDB 单步：CSEL 只有 1 条指令，无跳转

## 实验 5-3：CBZ/TBZ 位测试

**目标**：验证 CBZ/TBZ 的 1 条指令替代 2 条的效果。

```asm
; exp5_3.s
.global _start
_start:
    MOV x0, #0
    CBZ x0, zero_label       ; x0==0 → 跳转

    MOV x0, #-1              ; 0xFFFFFFFFFFFFFFFF
    TBNZ x0, #63, neg_label  ; bit63=1 → 跳转

zero_label:
    MOV x1, #1               ; 标记：走了零分支
neg_label:
    MOV x2, #2               ; 标记：走了负数分支
done:
    B done
```

**验证点**：
- GDB 中观察 CBZ 是否只执行 1 步就跳转（对比 CMP+B.EQ 需 2 步）
- TBNZ 直接测试 bit63，不需要构造掩码
- 检查 pstate 确认 CBZ/TBZ 不修改 NZCV

## 实验 5-4：条件后缀综合

**目标**：用错条件后缀（LT vs LO）观察错误结果。

```asm
; exp5_4.s
.global _start
_start:
    ; 两个"大"地址（高位为1）
    MOV x0, #0x8000000000000000   ; 内核空间地址 A
    MOV x1, #0x9000000000000000   ; 内核空间地址 B（A < B 无符号）

    CMP x0, x1

    ; 有符号判断：x0 是"负数"，x1 也是"负数"
    ; 0x80...0 = INT64_MIN, 0x90...0 = 负数
    ; INT64_MIN - 0x90...0 → 正数（溢出后符号翻转）
    ; N=0, V=1 → LT(N≠V=0≠1)成立 → "x0 < x1"
    ; 碰巧这里结果正确，但原因不同！

    ; 无符号判断：
    ; C=0(有借位) → LO(C=0)成立 → "x0 < x1"
    ; 这个总是正确的

    B.LT signed_result
    B unsigned_result
```

**验证点**：
- 对比 LT 和 LO 在地址比较中的结果
- 尝试混合用户地址(0x0000...)和内核地址(0xFFFF...)，观察 LT 的错误

## 实验 5-5：尾调用优化

**目标**：对比 `B func`（尾调用）和 `BL func; RET` 的栈帧差异。

```asm
; 普通调用
caller_normal:
    STP x29, x30, [sp, #-16]!    ; 保存 LR
    BL callee
    LDP x29, x30, [sp], #16      ; 恢复 LR
    RET                           ; 返回到 caller_normal 的调用者

; 尾调用
caller_tail:
    ; 不需要保存 LR（因为不返回到这里）
    B callee                      ; callee 的 RET 直接返回到 caller_tail 的调用者
```

**验证点**：
- GDB 中观察尾调用版本的 SP（栈指针）是否少了一层
- 观察 callee 的 RET 返回到哪里（尾调用返回到 caller_tail 的调用者）

## 自测题

1. 实验中如何验证 CSEL 确实无分支？
<details><summary>答案</summary>
GDB 单步执行：CSEL 只有一步，没有跳转。而 if-else 编译为 B.cond + B，至少两步且有跳转。也可看反汇编确认无 B 指令。用 `info registers pstate` 确认 CSEL 不修改 NZCV。
</details>

2. 如果用 LT 比较两个内核地址（0xFFFF...），会发生什么？
<details><summary>答案</summary>
地址高位为 1，有符号比较会把它们当成负数。如果两个地址都接近 0xFFFF...，LT 比较可能得到正确结果（都是"大负数"）；但混合用户地址(0x0000...)和内核地址时，用户地址被当成大正数，比较结果反转，导致 bug。
</details>

3. 如何在 GDB 中快速查看 NZCV 标志位？
<details><summary>答案</summary>
```
(gdb) p/x $pstate
```
NZCV 在 pstate 的最高 4 位：N=bit31, Z=bit30, C=bit29, V=bit28。例如 `0x60000000` 表示 Z=1, C=1（N=0, V=0）。也可以用 `(gdb) p (int)($pstate >> 28)` 查看高 4 位。
</details>

4. 在实验 5-2 中，`-O0` 和 `-O2` 编译 max_val 的差异是什么？
<details><summary>答案</summary>
`-O0`（不优化）会生成分支版本：
```asm
CMP w0, w1
B.LT else_branch
MOV w2, w0    ; if 分支
B done
else_branch:
MOV w2, w1    ; else 分支
done:
MOV w0, w2
```
`-O2`（优化）会生成 CSEL 版本：
```asm
CMP w0, w1
CSEL w0, w0, w1, GE
RET
```
优化后从 5+ 条指令减少到 2 条，且无分支。
</details>

5. 实验中如何验证尾调用省了一层栈帧？
<details><summary>答案</summary>
在 caller_normal 和 caller_tail 入口处记录 SP 值，进入 callee 后再查看 SP：
- 普通调用：callee 的 SP 比 caller_normal 的 SP 小 16（多一层栈帧）
- 尾调用：callee 的 SP 与 caller_tail 的 SP 相同（没有额外的栈帧）

也可以在 callee 的 RET 处用 `si` 单步，观察跳转目标：
- 普通调用：RET 回到 caller_normal 的 LDP 指令
- 尾调用：RET 直接回到 caller_tail 的调用者
</details>

## 参考与延伸

- 原书 §5.7
- [5.4 条件后缀](04-condition-suffix.md)
- [4.2 NZCV](../../chapter-04-a64-arithmetic-shift/notes/02-nzcv.md)
- [5.6 典型代码模式](06-patterns.md)
