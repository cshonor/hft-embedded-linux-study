# Ch7 完整总结 · A64 指令集的陷阱

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md) · [Pi5 适配](../../PI5-ADAPT.md)

---

## 本章定位

一线踩坑总结——把前 6 章学到的指令在实际工程中容易犯的错误集中讲。包含案例分析 + Linux 启动汇编大作业。

---

## 7.1 大立即数 MOV 陷阱

A64 的 `MOV` 不能直接加载任意 64 位立即数。

```asm
; ❌ 错误：汇编器报错
mov x0, #0x12345678deadbeef

; ✅ 正确方式1：LDR 伪指令（放 litpool）
ldr x0, =0x12345678deadbeef

; ✅ 正确方式2：MOVZ + MOVK 拆分
movz x0, #0xbeef, lsl #0      ; 低 16 位
movk x0, #0xdead, lsl #16     ; 次 16 位
movk x0, #0x5678, lsl #32     ; 再次 16 位
movk x0, #0x1234, lsl #48     ; 高 16 位
```

**MOV 立即数规则：**

| 方式 | 范围 |
|------|------|
| `MOVZ Xd, #imm16` | 16 位立即数，可 `lsl #0/16/32/48` |
| `MOVK Xd, #imm16, lsl #n` | 保留其他位，只改指定 16 位 |
| `MOV` (伪指令) | 汇编器自动拆成 MOVZ/MOVK |

> **陷阱**：写裸汇编时手动 `MOVZ+MOVK` 容易出错（位数、移位搞错）。优先用 `LDR =` 伪指令。

---

## 7.2 字符串加载陷阱

```asm
; 想加载字符串地址到寄存器
; ❌ 常见错误：MOV 不行
mov x0, #"Hello"   ; 报错

; ✅ 正确：ADR 或 LDR 伪指令
adr x0, msg        ; PC 相对寻址
ldr x0, =msg       ; 伪指令

msg:
    .asciz "Hello\n"
```

---

## 7.3 LDXR 导致死机 ⭐

```asm
; LDXR 后不做 STXR 也会标记监视器
; 如果 LDXR 后再次 LDXR（无配对 STXR），监视器状态可能异常
ldxr x0, [x1]
; ... 中间做了其他访存操作 ...
ldxr x0, [x1]      ; 某些实现可能死机或行为异常
```

**原因：**
- LDXR 设置本地独占监视器（Local Exclusive Monitor）
- 如果 LDXR 后发生异常、或中间访问了大量地址，监视器可能被清除
- 清除后再 STXR → 永远失败 → 死循环

**Ch7 案例 7-4：** 在树莓派 4B 上 LDXR 后不做 STXR 直接退出函数 → 独占监视器残留 → 后续 LDXR/STXR 异常。

**修复：** LDXR 后一定要配对 STXR（即使不需要写入，也要写回原值清除监视器）。

---

## 7.4 栈对齐陷阱

AArch64 **AAPCS64** 要求 SP 在函数入口处 **16 字节对齐**。

```asm
; ❌ 错误：只减 8
sub sp, sp, #8
str x0, [sp]          ; SP 不对齐，某些指令可能异常

; ✅ 正确：STP/LDP 自动 16 对齐
stp x29, x30, [sp, #-16]!
; ...
ldp x29, x30, [sp], #16
```

> STP/LDP 一次操作 2×64bit=16 字节，天然对齐。手动 `SUB SP, SP, #8` 破坏对齐。

---

## 7.5 条件执行陷阱

```asm
; ❌ 错误：以为 CMP 的标志会一直保持
cmp x0, x1
bl  other_func       ; BL 不改 NZCV，但 other_func 内部可能改
b.eq equal           ; 这里的 NZCV 可能已被 other_func 覆盖！

; ✅ 正确：比较后立即跳转，中间不调用其他函数
cmp x0, x1
b.eq equal
bl  other_func       ; 放在分支之后
```

> 只有**叶子函数**（不调用其他函数）才能安全地在 CMP 后做其他操作再跳转。  
> 稳妥做法：CMP 后紧跟 B.cond。

---

## 7.6 Linux 启动汇编分析（大作业 7-2）

Linux 内核 `arch/arm64/kernel/head.S` 启动流程：

```
_primary_entry:
  → 保存 bootloader 传递的参数（x0=dtb 地址）
  → 初步初始化（关 D-cache、设 SCTLR）
  → 设置初始栈
  → 创建恒等映射（identity mapping）
  → 开 MMU
  → 跳转到虚拟地址继续执行
  → start_kernel（C 代码）
```

**关键指令分析：**

| 步骤 | 关键指令 | 作用 |
|------|----------|------|
| 关中断 | `msr DAIFSet, #0xf` | 屏蔽所有中断 |
| 读取 EL | `mrs x0, CurrentEL` | 确认当前异常等级 |
| 设 SCTLR | `msr sctlr_el1, x0` | 关 MMU/DCache 准备配置 |
| 创建页表 | `adrp x0, init_pg_dir` | PC 相对寻址取页表 |
| 开 MMU | `msr sctlr_el1, x0` + `isb` | 使能 MMU |
| 地址跳转 | `br x0` | 跳到虚拟地址 |

> 读 `head.S` 是理解「汇编→C」启动链路的最佳练习。

---

## 7.7 串口输出实验

BenOS 串口输出（PL011 UART）核心：

```asm
; 轮询发送一个字符
; x0 = 字符, x1 = UART 基址
putchar:
    ldr w2, [x1, #0x18]    ; 读 FR (Flag Register)
    tbz w2, #5, tx_ready    # bit5=0 → TX FIFO 有空间
    b putchar               ; bit5=1 → 等待
tx_ready:
    str w0, [x1]            ; 写数据到 DR (Data Register)
    ret
```

> Pi4B: UART 基址 = 0xFE201000  
> **Pi5**: UART 基址 = 0x107C001000（BCM2712，不能照抄 4B）

---

## 7.8 易错点清单

1. **MOV 不能加载大立即数** → 用 `LDR =` 或 `MOVZ+MOVK`。
2. **LDXR 必须配对 STXR** → 否则监视器残留。
3. **SP 必须 16 字节对齐** → 用 STP/LDP，不要手动 `SUB SP, #8`。
4. **CMP 后不要调用函数** → 函数内部可能覆盖 NZCV。
5. **Pi5 UART 基址与 Pi4B 不同** → 照抄 4B 地址会死机。

---

## 书中思考题（自测）

1. 如何加载 64 位大立即数到寄存器？有哪两种方法？
2. LDXR 后不做 STXR 会有什么问题？
3. AAPCS64 对栈对齐有什么要求？为什么用 STP/LDP？
4. CMP 后调用 BL，之后还能用 B.eq 吗？为什么？
5. Linux 启动汇编中为什么要先创建恒等映射再开 MMU？

**参考答案：**

1. `LDR Xd, =imm`（伪指令+litpool）或 `MOVZ+MOVK` 拆分。  
2. 独占监视器残留 → 后续 LDXR/STXR **可能死机或永远失败**。  
3. **16 字节对齐**；STP/LDP 每次操作 2×64bit=16B，天然对齐。  
4. **不能安全使用**——BL 调用的函数内部可能执行 ADDS/SUBS 等指令覆盖 NZCV。  
5. 开 MMU 后 PC 仍是物理地址，需要恒等映射让物理地址 = 虚拟地址，否则取指失败。

---

上一章 [Ch6 杂项指令](../../chapter-06-a64-other-instructions/) · 下一章 [Ch8 GNU汇编器](../../chapter-08-gnu-assembler/) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md)
