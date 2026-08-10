# 7.1 大立即数 MOV 陷阱

> 来源：§7.1 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

MOV 指令无法加载任意 64 位立即数——这是从 ARM32 或 x86 迁移到 AArch64 时最常见的工程陷阱。

## 核心要点

### MOV 的立即数限制

AArch64 的 MOV 指令（实际是 MOVZ/MOVN 的别名）**只能加载 16 位立即数**，可选配合移位：

```
MOVZ Xd, #imm16              ; Xd = imm16（零扩展）
MOVZ Xd, #imm16, LSL #16     ; Xd = imm16 << 16（其他位清零）
MOVZ Xd, #imm16, LSL #32     ; Xd = imm16 << 32
MOVZ Xd, #imm16, LSL #48     ; Xd = imm16 << 48

MOVN Xd, #imm16              ; Xd = ~imm16（取反，用于 0xFFF...F 等）
MOVK Xd, #imm16, LSL #N      ; Xd[N+15:N] = imm16（保留其他位，Keep）
```

### 可编码的立即数范围

| 立即数 | 单条指令 | 编码方式 |
|--------|----------|----------|
| 0x0000 - 0xFFFF | MOVZ | 直接 16 位 |
| 0x00010000 - 0xFFFF0000 | MOVZ LSL #16 | 16位左移16 |
| 0x0000000100000000 - 0xFFFF000000000000 | MOVZ LSL #32/48 | 16位左移32/48 |
| 0xFFFFFFFFFFFF0000 | MOVN #0x0000 | ~0 = 0xFFF...F, ~0xFFFF = 0xFFF...F0000 |
| 其他 | **不可单条编码** | 需 MOVZ+MOVK |

### MOVZ/MOVK 组合加载大常量

```asm
; 加载 0x12345678（32位）
MOVZ X0, #0x5678             ; X0 = 0x0000000000005678
MOVK X0, #0x1234, LSL #16   ; X0 = 0x0000000012345678（保留低16位）

; 加载 0xDEADBEEFCAFEBABE（64位）
MOVZ X0, #0xBABE                    ; X0 = 0x000000000000BABE
MOVK X0, #0xCAFE, LSL #16          ; X0 = 0x00000000CAFEBABE
MOVK X0, #0xBEEF, LSL #32          ; X0 = 0x0000BEEFCAFEBABE
MOVK X0, #0xDEAD, LSL #48          ; X0 = 0xDEADBEEFCAFEBABE

; 加载 0xFFFFFFFFFFFFFFFF（全1）
MOVN X0, #0                  ; X0 = ~0 = 0xFFFFFFFFFFFFFFFF（1条！）

; 加载 0xFFFFFFF0（高位全1，低4位为0）
MOVN X0, #0x0F, LSL #0      ; X0 = ~0x0F = 0xFFFFFFFFFFFFFFF0
```

### LDR =伪指令

```asm
; 用 LDR =加载任意常量（汇编器自动生成文字池）
LDR X0, =0x12345678         ; 汇编器在附近放一个 .quad 0x12345678
                              ; 编译为 LDR X0, [PC, #offset]
                              ; 需要额外 4-8 字节文字池 + 1 次内存访问

; LDR =也可加载符号地址
LDR X0, =label               ; 加载 label 的地址
```

### MOVZ+MOVK vs LDR =对比

| 特性 | MOVZ+MOVK | LDR =伪指令 |
|------|-----------|-------------|
| 指令数 | 2-4 条 | 1 条 |
| 内存访问 | 无 | 1次（文字池） |
| 延迟 | 固定（2-4 cycle） | 不定（可能 cache miss ~12+ cycle） |
| 代码体积 | 8-16 字节 | 4 字节 + 8 字节文字池 |
| 适用场景 | HFT 热路径 | 非热路径、方便快捷 |

### MOV 立即数的硬件编码原理

```
AArch64 指令固定 32 位。MOVZ/MOVK 的编码：
  [31]    sf: 0=32位, 1=64位
  [30:29] opc: 10=MOVZ, 11=MOVK, 00=MOVN
  [28:23] 100101
  [22:21] hw: 移位量（00=0, 01=16, 10=32, 11=48）
  [20:5]  imm16: 16位立即数
  [4:0]   Rd: 目标寄存器

→ 只有 16 位给立即数，所以单条指令最多编码 16 位值
→ x86 的 MOV 可以编码 32/64 位立即数（指令长度可变）
→ ARM 是定长指令集，无法在 32 位指令中编码大立即数
```

## 与 C 的对照

```c
// C 代码中的大常量
uint64_t magic = 0xDEADBEEFCAFEBABE;

// 编译器自动选择最优编码
// -O2: 4 条 MOVZ/MOVK（无内存访问）
// -O0: 可能用 LDR =（简单但慢）
```

## 常见错误

1. **内联汇编直接写大立即数**：`asm volatile("mov x0, #0x12345678")` → 汇编报错。
2. **误以为 MOV 无限制**：从 x86/ARM32 迁移时，x86 的 `mov rax, 0x12345678` 合法，AArch64 不合法。
3. **MOVN 计算错误**：`MOVN X0, #0` = 全1（0xFFF...F），不是 0。MOVN 是"取反移动"。

## HFT 关联

大常量加载的性能影响：
- MOVZ+MOVK 两条指令 → 2 cycle，无内存访问，延迟可预测
- LDR =伪指令 → 1 条指令但需访问文字池 → 可能 cache miss（~12+ cycles）
- HFT 热路径优先用 MOVZ/MOVK 组合（可预测延迟）
- 编译器自动选择最优方式，但内联汇编需手动处理

```asm
; HFT：热路径中加载魔数（用 MOVZ/MOVK 确保无 cache miss）
MOVZ X0, #0xBEEF
MOVK X0, #0xDEAD, LSL #16
; 2 cycle 固定延迟，无不确定性

; 反模式：热路径中用 LDR =（可能 cache miss）
LDR X0, =0xDEADBEEF     ; 如果文字池不在 L1 → 12+ cycle 不可预测
```

## 自测题

1. 以下指令哪些合法？
```asm
mov x0, #0x1000
mov x0, #0x10000
mov x0, #0x10001
```
<details><summary>答案</summary>
- `mov x0, #0x1000` 合法（16位内）
- `mov x0, #0x10000` 合法（=1<<16，可移位表示）
- `mov x0, #0x10001` **不合法**（不能单条 MOV 表示，需 MOVZ+MOVK）
</details>

2. 用 MOVZ/MOVK 加载 0xDEADBEEF 到 x0。
<details><summary>答案</summary>
```asm
movz x0, #0xBEEF
movk x0, #0xDEAD, lsl #16
```
</details>

3. MOVZ+MOVK 和 LDR =哪个更适合 HFT 热路径？
<details><summary>答案</summary>
MOVZ+MOVK 更适合。两条指令 2 cycle，无内存依赖，延迟可预测。LDR =需要访问文字池（内存），可能 cache miss 导致 ~12+ cycle 不可预测延迟。但 LDR =只需 1 条指令，代码更紧凑，适合非热路径。
</details>

4. 如何用最少指令加载 0xFFFFFFFFFFFFFFF0 到 x0？
<details><summary>答案</summary>
```asm
MOVN X0, #0x0F      ; X0 = ~0x0F = 0xFFFFFFFFFFFFFFF0（1条指令！）
```
MOVN 取反 16 位立即数 0x000F → 0xFFFFFFFFFFFFFFF0。利用 MOVN 可以用 1 条指令加载大量"高位为1"的常量。
</details>

5. 为什么 AArch64 的 MOV 不能像 x86 那样编码 64 位立即数？
<details><summary>答案</summary>
AArch64 是定长指令集（所有指令 32 位）。MOVZ/MOVK 的 32 位编码中，只有 16 位留给立即数（其余是操作码、寄存器号、移位量）。x86 是变长指令集，MOV 可以扩展到 10+ 字节，容纳完整的 64 位立即数。这是 RISC（定长）和 CISC（变长）的经典设计差异。
</details>

## 参考与延伸

- 原书 §7.1
- [3.5 LDR 伪指令](../../chapter-03-a64-load-store/notes/section-0-本章完整概述.md)
- [4.4 位操作（MOVZ/MOVK/MOVN 编码）](../../chapter-04-a64-arithmetic-shift/notes/04-bit-ops.md)
