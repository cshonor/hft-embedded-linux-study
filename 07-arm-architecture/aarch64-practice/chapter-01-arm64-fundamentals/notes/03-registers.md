# 1.3 AArch64 寄存器组

> 来源：§1.3 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

AArch64 有 31 个 64 位通用寄存器 X0-X30（X=64 位，W=低 32 位）、每 EL 独立的特殊寄存器（SP/ELR/SPSR）、以及 PSTATE 处理器状态。本节覆盖 AAPCS64 调用约定、SP 选择机制、异常保存/恢复的硬件动作。

## 核心要点

### 一、通用寄存器 X0-X30（AAPCS64 调用约定）

31 个通用寄存器，编码 0-30。编码 31 不是 X31，而是 **XZR/WZR 零寄存器**（只读恒 0，写被丢弃）。

| 寄存器 | 32位别名 | 用途 | 保存规则 |
|--------|----------|------|----------|
| X0 | W0 | 第1参数 / 函数返回值 | Caller-saved |
| X1 | W1 | 第2参数 / 64位返回值高半部分 | Caller-saved |
| X2-X7 | W2-W7 | 第3-8参数 | Caller-saved |
| X8 | W8 | 间接返回地址（大结构体返回指针） | Caller-saved |
| X9-X15 | W9-W15 | 临时寄存器 | Caller-saved |
| X16-X17 | W16-W17 | IP0/IP1，PLT 跳转临时 | Caller-saved |
| X18 | W18 | **平台寄存器**（Linux 内核保留，存 current） | 平台定义 |
| X19-X28 | W19-W28 | 保存寄存器 | **Callee-saved** |
| X29 | W29 | FP 帧指针 | **Callee-saved** |
| X30 | W30 | LR 链接寄存器（BL 返回地址） | **Callee-saved** |
| XZR | WZR | 零寄存器（编码31，只读恒0） | — |

**Caller-saved vs Callee-saved 区分：**

- **Caller-saved（X0-X17）**：调用方负责保存。如果调用方还要用，自己在调用子函数前压栈；子函数可以随便改写。
- **Callee-saved（X19-X30）**：被调用方负责保存。子函数如果要使用，必须先压栈，返回前恢复原值。
- **X18（平台寄存器）**：AAPCS64 定义为平台寄存器，保存规则由平台决定。Linux 内核将其**保留**用于存储 `current`（当前 task_struct 指针），不应在普通代码中使用；Linux 用户态中保守地避开它。

**X8 间接返回地址：** 当函数返回**大于 16 字节的结构体**时，调用方在 X8 中放一个指针，被调用方把返回值写到该地址。与 X0（普通标量返回值）配合。

**X30(LR) vs ELR_ELx：** 普通函数调用 `BL` 把返回地址存入 X30；**异常返回地址不存 X30，存 ELR_ELx**。这是新手最常混淆的点。

### 新手补充：XZR 零寄存器详解

> **一句话记忆：XZR 是一个"假"寄存器——读它永远得到 0，写它等于什么都没做。**

ARM64 有 31 个通用寄存器 X0～X30，**没有 X31**。编码值 31 的位置被设计成两个特殊东西：

| 编码值 | 64 位名字 | 32 位名字 | 读 | 写 |
|--------|-----------|-----------|----|----|
| 31 | **XZR** | **WZR** | 永远返回 0 | **丢弃**，不保存 |

#### 为什么需要 XZR？

ARM64 是**定长编码**，每条算术指令的编码里都必须填一个目标寄存器字段。但有时候你根本不想要计算结果——比如 CMP 你只想更新标志位。这时把目标填成 XZR（编码 31），结果就被丢弃了。

**没有 XZR 会怎样？** 你得浪费一个真实寄存器存临时垃圾，然后再处理它。XZR 省掉了这个麻烦。

#### XZR 的 5 大用途

**用途 1：做比较——CMP 的核心**

```asm
cmp x0, x1        ; 比较两个数，不要结果，只要标志位
                  ; 硬件实际编码：subs xzr, x0, x1
                  ; 结果扔进 XZR（垃圾桶），NZCV 留着
```

**用途 2：做无副作用测试**

```asm
; 只测 x1 是不是 0，不占任何寄存器存结果
subs xzr, x1, #0  ; 等效于 cmp x1, #0，结果丢弃
b.ne not_zero      ; x1 != 0 就跳转
```

**用途 3：用 0 参与运算（MOV/MUL 的真实编码）**

```asm
mov x0, xzr        ; x0 = 0（用 XZR 给寄存器清零）
                    ; 真实编码：orr x0, xzr, #0

mul x0, x1, x2     ; x0 = x1 * x2
                    ; 真实编码：madd x0, x1, x2, xzr
                    ;           x0 = x1*x2 + 0（加数用 XZR = 0）
```

**用途 4：占位——指令格式需要填满字段**

ARM64 每条算术指令都必须写满寄存器字段。MADD Rd, Rn, Rm, Ra 四个操作数不能少，但有时你不需要 Ra 那个加数：

```asm
; 你要的是 x0 = x1 * x2，没有额外的加数
madd x0, x1, x2, xzr   ; Ra 填 XZR，加 0，等效于纯乘法
```

**用途 5：清内存**

```asm
str xzr, [x0]      ; 把 x0 指向的内存写 0（用 XZR 当源操作数）
```

#### XZR 速查表

| 场景 | 用法 | 为什么用 XZR |
|------|------|-------------|
| CMP/CMN | 目标填 XZR | 只要标志位，不要结果 |
| 清零 | `mov x0, xzr` | 拿 0 当源，比加载立即数 0 快 |
| MUL | MADD 第四操作数填 XZR | 指令格式要求填满，XZR = 加 0 |
| 无副作用测试 | `subs xzr, ...` | 只看标志不占寄存器 |
| MOV 别名 | `orr x0, xzr, ...` | ORR 配 XZR 等于复制 |
| 清内存 | `str xzr, [x0]` | 用 0 当写入源 |

> **新手关键理解：** XZR 本质上解决一个问题——ARM64 每条算术指令都必须写一个目标寄存器，但有时候你不需要结果，XZR 就是那个"不需要"时的垃圾桶。CMP、CMN、MUL、MOV 这些"别名指令"之所以能映射到真实的 SUBS/ADDS/MADD/ORR 编码上，全靠 XZR 填空。

### 二、栈指针 SP_EL0 / SP_EL1 / SP_EL2 / SP_EL3

每个异常级别拥有独立栈指针：

| 寄存器 | 用途 |
|--------|------|
| SP_EL0 | EL0 用户态栈 |
| SP_EL1 | EL1 内核栈 |
| SP_EL2 | EL2 Hypervisor 栈 |
| SP_EL3 | EL3 Monitor 固件栈 |

**SP 选择机制（PSTATE.SP 位）：**

| PSTATE.SP | 模式 | 当前 EL 使用 | 说明 |
|-----------|------|-------------|------|
| 0 | ELt | SP_EL0 | 复用用户栈（调试场景） |
| 1 | ELh | SP_ELx | 本级别专属栈（**Linux 内核默认**） |

- EL0 只能访问 SP_EL0
- 异常发生时硬件自动切换到目标 EL 对应的 SP_ELx
- SP 不是 X 寄存器，不能当作通用寄存器自由使用

### 三、ELR_ELx 异常链接寄存器

ELR_EL1 / ELR_EL2 / ELR_EL3，64 位，每个可接收异常的特权 EL 一个（EL0 不接收异常，无 ELR_EL0）。

- **异常进入**：硬件自动把触发异常的 PC 写入 ELR_ELx
- **ERET 返回**：硬件自动从 ELR_ELx 加载到 PC，回到被打断的代码

> **区分：** 普通函数调用返回地址 → X30(LR)；异常（SVC/HVC/SMC/IRQ）返回地址 → ELR_ELx。两者完全不同。

### 四、SPSR_ELx 保存的处理器状态

SPSR_EL1 / SPSR_EL2 / SPSR_EL3，32 位，每个可接收异常的 EL 一个。

- **异常进入**：硬件自动把当前 PSTATE 完整复制到 SPSR_ELx
- **ERET 返回**：硬件自动把 SPSR_ELx 写回 PSTATE，恢复标志位、中断屏蔽、异常等级

### 五、PSTATE 处理器状态

> **PSTATE 不是一个物理寄存器！** 它是一组状态位的抽象集合。没有 PSTATE_ELx 硬件寄存器。异常时硬件把当前 PSTATE 完整拷贝到 SPSR_ELx（这才是物理寄存器）保存；ERET 再从 SPSR_ELx 恢复。

| 位域 | 名称 | 含义 |
|------|------|------|
| [31] | N | Negative — 结果为负（有符号） |
| [30] | Z | Zero — 结果等于 0 |
| [29] | C | Carry — 进位/借位（无符号） |
| [28] | V | Overflow — 有符号溢出 |
| [9] | D | Debug 异常屏蔽（置1=屏蔽） |
| [8] | A | SError 系统错误屏蔽 |
| [7] | I | IRQ 普通中断屏蔽 |
| [6] | F | FIQ 快速中断屏蔽 |
| [4:0] | M[3:0] | 当前异常等级 EL0-EL3 |
| — | nRW | 0=AArch64 / 1=AArch32（目标 EL 执行状态） |
| — | SP | 0=SP_EL0 / 1=SP_ELx（栈指针选择） |

**DAIF 记忆口诀：** D(调试) → A(SError) → I(IRQ) → F(FIQ)，bit9→bit6 从高到低。Linux 内核用 `MSR DAIFSet, #imm` / `DAIFClr` 一次性开关全部中断。

### 六、SVC 异常硬件动作串联（完整流程）

以 EL0 用户程序执行 `SVC #0` 系统调用为例：

```
EL0 用户程序执行 SVC #0（同步异常）
    │
    ▼ 硬件自动执行（软件不用写）：
    ├── PC → ELR_EL1          （保存返回地址）
    ├── PSTATE → SPSR_EL1     （保存处理器状态）
    ├── PSTATE.M → EL1        （切换到目标异常等级）
    ├── PSTATE.DAIF → 1       （屏蔽中断）
    ├── SP → SP_EL1           （切换到内核栈）
    └── PC → VBAR_EL1+offset  （跳到异常向量表入口）
    │
    ▼ EL1 内核处理系统调用
    │
    ▼ 执行 ERET 指令：
    ├── PC ← ELR_EL1          （恢复返回地址）
    └── PSTATE ← SPSR_EL1     （恢复状态，切回 EL0）
    │
    ▼ 回到 EL0 用户态继续执行
```

> **关键记忆：** ERET 做两件大事——恢复 PC 来自 ELR_ELx；恢复 PSTATE 来自 SPSR_ELx。原子操作，不可被打断。**X0-X30 硬件不保存，软件必须手动压栈。**

## HFT 关联

| 场景 | 寄存器 | HFT 影响 |
|------|--------|----------|
| 参数传递 | X0-X7 | 关键路径函数参数不超过 8 个，避免栈传参的额外访存 |
| 帧指针 | X29 | 热路径用 `-fomit-frame-pointer` 省掉 FP 保存/恢复（2 条访存） |
| 链接寄存器 | X30 | 叶子函数（不调用其他函数）不需要保存 LR，编译器自动优化 |
| 中断屏蔽 | DAIF.I | HFT 关键路段 `local_irq_disable()` 防止 IRQ 抢占引入抖动 |
| 零寄存器 | XZR | `MOV Xn, XZR` 比显式加载 0 快，编译器自动利用 |
| 异常保存 | ELR/SPSR | SVC 系统调用约 1-3μs，HFT 用共享内存 + busy-polling 替代 |
| 通用寄存器 | X0-X30 | 异常保存/恢复 31 个寄存器约 32 条 STP/LDP ≈ 30-50ns（A76） |

## 自测题

1. **写 W0 后，X0 的高 32 位会保留吗？**

<details>
<summary>答案</summary>

不会。写 W0 会自动清零 X0 的高 32 位。这是 AArch64 硬件行为，不同于 AArch32（ARMv7 写低 32 位不影响高 32 位）。
</details>

2. **XZR 和 WZR 分别是什么？有什么用途？**

<details>
<summary>答案</summary>

零寄存器（编码 31），读永远为 0，写被丢弃。用途：比较（`CMP Xn, XZR`）、清零（`MOV Xn, XZR`）、替代不需要的目的寄存器（如 `STR XZR, [X0]` 清内存）。
</details>

3. **异常发生时，硬件自动保存哪些寄存器？哪些需要软件保存？**

<details>
<summary>答案</summary>

硬件自动保存：**ELR_ELx**（PC 值）和 **SPSR_ELx**（PSTATE 值），同时切换 SP 到目标 EL 的 SP_ELx。软件必须保存：**X0-X30**（31 个通用寄存器），因为硬件不自动保存，ISR 可能覆盖它们。
</details>

4. **X18 在 AAPCS64 中的保存规则是什么？Linux 内核如何使用它？**

<details>
<summary>答案</summary>

AAPCS64 定义 X18 为**平台寄存器**，保存规则由平台决定，**不是通用的 callee-saved**。Linux 内核将其**保留**，用于存储 `current`（当前 task_struct 指针）；Linux 用户态中保守地避开使用 X18。普通代码不应碰 X18。
</details>

5. **函数调用的返回地址和异常的返回地址分别存在哪里？为什么不同？**

<details>
<summary>答案</summary>

- 函数调用（`BL` 指令）：返回地址存入 **X30(LR)**，因为函数调用是同步的、可预期的，用 LR 即可
- 异常（SVC/HVC/SMC/IRQ）：返回地址存入 **ELR_ELx**，因为异常可能打断任意代码流，需要独立于 LR 的专用寄存器保存

如果异常也用 LR 保存，会覆盖当前函数的 LR，导致返回链断裂。
</details>

6. **PSTATE 是真实存在的物理寄存器吗？异常时它被保存到哪里？**

<details>
<summary>答案</summary>

**不是。** PSTATE 是一组状态位的抽象集合（NZCV、DAIF、M、nRW、SP 等），没有 PSTATE_ELx 物理寄存器。异常发生时，硬件把当前 PSTATE **完整拷贝到 SPSR_ELx**（这才是物理寄存器）保存；ERET 时从 SPSR_ELx 恢复回 PSTATE。
</details>

7. **PSTATE.SP=0 和 PSTATE.SP=1 分别选择哪个栈指针？Linux 内核默认用哪个？**

<details>
<summary>答案</summary>

- SP=0（ELt 模式）：当前 EL 使用 **SP_EL0**（复用用户栈）
- SP=1（ELh 模式）：当前 EL 使用 **SP_ELx**（本级别专属栈）

Linux 内核默认 SP=1（ELh 模式），用 SP_EL1 作为内核栈。某些调试场景临时切到 SP=0 复用用户栈。
</details>

8. **EL0 发生 IRQ 中断进入 EL1，PSTATE 被保存到哪个寄存器？PC 被保存到哪个寄存器？**

<details>
<summary>答案</summary>

- PSTATE → **SPSR_EL1**（保存被打断时的处理器状态）
- PC → **ELR_EL1**（保存下一条要执行的指令地址，异步异常指向下一条指令）

ERET 返回时：PC ← ELR_EL1，PSTATE ← SPSR_EL1，原子切回 EL0。
</details>

## 参考与延伸

- 原书 §1.3
- [NZCV 条件码专篇](../../NZCV.md) — 条件码如何在比较指令中设置
- [§1.2 异常等级](02-exception-levels.md) — EL0-EL3 四级特权
- [Ch11 §11.2 EL 切换](../../chapter-11-exception-handling/notes/02-el-switch.md) — SVC/HVC/SMC/ERET 切换指令
- [Ch11 §11.4 硬件保存+软件保存](../../chapter-11-exception-handling/notes/04-hw-sw-save.md) — 异常现场保存/恢复代码
