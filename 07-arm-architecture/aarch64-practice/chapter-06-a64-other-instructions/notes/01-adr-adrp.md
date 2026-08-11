# 6.1 ADR / ADRP 内核重定位关键

> 来源：§6.1 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

ADR 和 ADRP 指令——PC 相对地址加载，内核重定位和位置无关代码（PIC）的关键。这两条指令是 AArch64 获取地址的核心方式。

## 核心要点

### 指令对比

| 指令 | 计算方式 | 结果精度 | 范围 | 典型用途 |
|------|----------|----------|------|----------|
| ADR Xd, label | PC + imm21 | 精确地址 | ±1MB | 短距离地址加载 |
| ADRP Xd, label | (PC & ~0xFFF) + (imm21 << 12) | 页对齐(低12位=0) | ±4GB | 全局变量/函数地址 |

### ADR 详解

```asm
; ADR 计算 PC + 偏移，得到精确地址
; 偏移范围：±1MB（21 位有符号偏移）

adr x0, label          ; x0 = PC + offset_to_label
adr x0, .              ; x0 = 当前指令地址（常用技巧）

; 典型用途：获取附近的数据地址
.data
msg:
    .ascii "Hello\0"
.text
func:
    adr x0, msg        ; 获取 msg 地址（偏移必须在 ±1MB 内）
    bl printf
```

### ADRP 详解

```asm
; ADRP 计算 (PC & ~0xFFF) + (imm21 << 12)
; 即：当前 PC 所在的 4KB 页基地址 + 页偏移
; 结果的低 12 位总是 0（页对齐）

adrp x0, global_var    ; x0 = global_var 所在页的基地址
add x0, x0, :lo12:global_var  ; x0 += global_var & 0xFFF（页内偏移）

; 为什么要两条指令？
; ADRP 获取页地址（范围 ±4GB），ADD 补全页内偏移
; 分两步是因为单条指令无法同时编码 4GB 范围和 12 位精度
```

### ADRP+ADD 的完整模式

```asm
; 获取全局变量地址（最常见模式）
adrp x0, global_var
add x0, x0, :lo12:global_var
ldr x1, [x0]           ; 读取全局变量值

; 获取全局变量地址并直接 LDR（编译器优化）
adrp x0, global_var
ldr x1, [x0, :lo12:global_var]  ; ADRP + 带偏移的 LDR

; 获取字符串地址
adrp x0, msg
add x0, x0, :lo12:msg
bl puts
```

### :lo12: 伪操作详解

```
:lo12:label 表示 label 地址的低 12 位（页内偏移）

例如 label = 0x4005A0：
  ADRP x0, label  → x0 = 0x400000（页基地址，低12位清零）
  :lo12:label     = 0x5A0（低12位偏移）
  ADD x0, x0, #0x5A0 → x0 = 0x4005A0（精确地址）

注意：ADD 立即数范围是 0-4095（12位），正好是页内偏移范围。
```

### ADR vs LDR =伪指令

```asm
; 方法1：LDR =伪指令（从文字池加载地址）
ldr x0, =label        ; 汇编器在附近放一个文字池，存 label 地址
                       ; 编译为 LDR x0, [PC+offset] 读文字池
                       ; 额外占用 4-8 字节文字池空间 + 1 次内存访问

; 方法2：ADRP+ADD（PC 相对计算）
adrp x0, label
add x0, x0, :lo12:label
; 2 条指令，无额外内存访问，更高效

; 方法3：ADR（短距离）
adr x0, label          ; 1 条指令，但范围只有 ±1MB
```

| 方式 | 指令数 | 额外内存访问 | 范围 | 推荐度 |
|------|--------|-------------|------|--------|
| ADR | 1 | 无 | ±1MB | 短距离首选 |
| ADRP+ADD | 2 | 无 | ±4GB | 长距离首选 |
| LDR = | 1 | 1次(文字池) | 任意 | 不推荐（有额外访存） |

### ADR / ADRP 指令编码分析

ADR 和 ADRP 都是 32 位固定长度指令，21 位有符号偏移拆成两段编码：

```
ADR 指令格式（bit31 = 0）：
  [31]  op = 0（ADR）
  [30:29] immlo（偏移低 2 位）
  [28:24] 10000（固定操作码）
  [23:5]  immhi（偏移高 19 位）
  [4:0]   Rd（目标寄存器）

ADRP 指令格式（bit31 = 1）：
  [31]  op = 1（ADRP）
  [30:29] immlo（偏移低 2 位）
  [28:24] 10000（固定操作码）
  [23:5]  immhi（偏移高 19 位）
  [4:0]   Rd（目标寄存器）

偏移组装：imm21 = immhi:immlo = [19:2] + [1:0]
  ADR：  offset = sign_extend(imm21)              → 字节粒度，范围 ±1MB
  ADRP： offset = sign_extend(imm21) << 12        → 页粒度，范围 ±4GB
```

**关键差异只有 bit[31]**：0 = ADR（字节偏移），1 = ADRP（页偏移）。其余编码完全相同。这意味着：
- ADR 和 ADRP 共享同一套硬件解码电路，只是偏移解释不同
- 21 位偏移 × 2（ADR 字节粒度 vs ADRP 页粒度）= ADRP 范围是 ADR 的 4096 倍

### ADRP 地址计算全流程

用一个具体例子完整走一遍：

```
假设：
  当前指令地址 PC = 0xFFFF800010000420
  目标全局变量 global_var = 0xFFFF800010005A0

第 1 步：ADRP 计算
  PC 页基址 = PC & ~0xFFF = 0xFFFF800010000000
  目标页基址 = 0xFFFF800010005A0 & ~0xFFF = 0xFFFF800010000000（同一页！）
  
  偏移 = 目标页基址 - PC 页基址 = 0x0000000000000000
  编码 imm21 = 0 >> 12 = 0
  
  ADRP x0, global_var → x0 = 0xFFFF800010000000（页基地址，低12位=0）

第 2 步：ADD 补全页内偏移
  :lo12:global_var = 0xFFFF800010005A0 & 0xFFF = 0x5A0
  
  ADD x0, x0, #0x5A0 → x0 = 0xFFFF8000100005A0（精确地址）✓

如果 global_var 在另一页（0xFFFF800010012000）：
  PC 页基址 = 0xFFFF800010000000
  目标页基址 = 0xFFFF800010010000
  偏移 = 0x10000 → imm21 = 0x10000 >> 12 = 0x10
  
  ADRP x0, global_var → x0 = 0xFFFF800010010000
  ADD x0, x0, #0x200  → x0 = 0xFFFF800010012000 ✓
```

### 为什么 ADRP 要页对齐 PC

```
ADR 编码：imm21 是字节偏移，直接加到 PC
  → PC + imm21，精度到单个字节，但范围只有 ±1MB（21 位）

ADRP 编码：imm21 是页偏移，左移 12 位后加到 PC
  → (PC & ~0xFFF) + (imm21 << 12)
  → 范围 ±4GB（21 位 × 4KB），但精度只有 4KB

为什么 PC 也要清零低 12 位？
  因为偏移以页为单位（<< 12），如果 PC 不对齐：
    PC = 0x420, offset = 0x1000
    PC + offset = 0x1420 ← 这是"PC + 1页"，但 0x420 的页基址是 0x000
    正确应该是 0x000 + 0x1000 = 0x1000，然后加 0x420 的页内偏移
  
  清零 PC 低 12 位后：
    PC_base = 0x000, offset = 0x1000
    result = 0x1000 ← 页基地址，低 12 位为 0
    再用 ADD :lo12: 补全 → 0x1000 + 0x420 = 0x1420

本质：ADR 和 ADRP 各承担一半精度
  ADR：21 位字节偏移 → 精度高（字节级）但范围小（±1MB）
  ADRP：21 位页偏移 → 范围大（±4GB）但精度低（页级）
  ADRP + ADD :lo12: → 范围大且精度高，用 2 条指令实现 4GB 范围内任意地址加载
```

> 这就是 AArch64 的设计哲学——32 位固定指令长度放不下 64 位绝对地址，用 PC 相对 + 两步拼装实现位置无关寻址。ADR 是"近距离精确版"，ADRP 是"远距离粗略版"，二者配合覆盖全场景。

## 与 C 的对照

```c
// C 代码中的全局变量访问
extern int global_var;
int *ptr = &global_var;     // 编译器生成 ADRP+ADD
int val = global_var;       // 编译器生成 ADRP+LDR
```

```asm
// 编译器自动生成
adrp x0, global_var
add x0, x0, :lo12:global_var    ; ptr = &global_var

adrp x0, global_var
ldr w1, [x0, :lo12:global_var]  ; val = global_var
```

## 常见错误

1. **ADRP 后忘记 ADD :lo12:**：得到的是页地址（低12位=0），不是精确地址。
2. **ADR 超出 ±1MB**：ADR 偏移只有 21 位，超出范围链接器报错。改用 ADRP+ADD。
3. **混淆 ADRP 的 PC 对齐**：ADRP 用的是 `(PC & ~0xFFF)`，不是 PC 本身。所以 ADRP 结果不等于 ADR 结果的高位。

## HFT 关联

ADRP 在共享库和位置无关代码中至关重要：
- 交易引擎作为共享库加载时，全局变量访问用 ADRP+ADD → 支持 ASLR
- 内核模块（KO）加载用 ADRP 获取模块内数据 → 支持动态加载
- ADRP+ADD 比 LDR =伪指令更高效（不依赖文字池，无额外内存访问）
- 但 ADRP 跨页时需注意地址范围限制（±4GB）
- 内核 KASLR（地址随机化）依赖 ADRP 的 PC 相对寻址

```asm
; HFT：位置无关的全局计数器访问
adrp x0, trade_counter
ldr x1, [x0, :lo12:trade_counter]  ; 读取计数器
add x1, x1, #1
str x1, [x0, :lo12:trade_counter]  ; 写回
```

## 自测题

1. ADR 和 ADRP 的区别？
<details><summary>答案</summary>
ADR 计算精确地址（PC + 偏移），范围 ±1MB。ADRP 计算页对齐地址（PC 页基址 + 偏移<<12），范围 ±4GB。ADRP 得到的地址低 12 位为 0，需要配合 ADD :lo12: 补全。
</details>

2. 为什么内核 KASLR 需要 ADRP 而不能用绝对地址？
<details><summary>答案</summary>
KASLR（内核地址空间随机化）在每次启动时改变内核加载地址。绝对地址在编译时固定，无法适应运行时重定位。ADRP 是 PC 相对寻址，不依赖绝对地址，内核代码无论加载到哪里都能正确找到自己的数据。
</details>

3. 以下代码的完整作用是什么？
```asm
adrp x0, global_var
add x0, x0, :lo12:global_var
```
<details><summary>答案</summary>
获取 global_var 的运行时地址到 x0。ADRP 获取 global_var 所在的 4KB 页基地址，ADD 补全页内偏移（低 12 位）。这是 AArch64 获取全局变量地址的标准模式。
</details>

4. 为什么 `ldr x0, =label` 不如 `adrp+add` 高效？
<details><summary>答案</summary>
LDR =伪指令需要在代码附近放置一个"文字池"（literal pool）存储 label 的地址值，然后 LDR 从文字池加载 → 额外的内存访问（可能 cache miss）。ADRP+ADD 是纯计算指令，不需要额外内存访问。虽然 ADRP+ADD 是 2 条指令，但无数据依赖的访存，总延迟通常更低。
</details>

5. ADRP 的 PC 对齐是什么意思？为什么不是直接用 PC？
<details><summary>答案</summary>
ADRP 用的是 `(PC & ~0xFFF)`，即将 PC 的低 12 位清零（4KB 页对齐）。因为 ADRP 的偏移以页（4KB=0x1000）为单位编码（imm21 << 12），所以基准地址也必须是页对齐的。这样 ADRP 结果 = 页基地址 + 页偏移，低 12 位为 0，需要 ADD :lo12: 补全页内偏移。如果直接用 PC（非对齐），偏移计算会出错。
</details>

## 参考与延伸

- 原书 §6.1
- [Ch9 链接脚本](../../chapter-09-linker-scripts/notes/section-0-本章完整概述.md)
- [Ch14 MMU 页表](../../chapter-14-memory-management/notes/section-0-本章完整概述.md)
