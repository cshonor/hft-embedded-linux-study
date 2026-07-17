## 3.3–3.4 数据格式与访问信息

> **CSAPP 全书基准：** **x86-64（AMD64）+ Linux gas（AT&T）+ System V ABI**。  
> **HFT：只练 AT&T** — 共置机在 Linux；`perf`/`objdump` 默认即此，**不学 Intel 语法**。  
> 与 ARM32 语法不通用（对照 → [Smith §2.2](../../../19-ARM64-Architecture/arm32-smith-assembly/chapter-02-programmers-model/notes/section-2-2-data-types.md)）。

> **一条线：** 后缀 `b/w/l/q` = 操作多宽；操作数写法 = 值从哪来 — **ABI 在机器码层的底座**。

→ ABI：[Ch2 · ABI](../../chapter-02-representing-information/notes/section-2.1.2-abi-application-binary-interface.md) · 传参：[§3.7](./section-3.7-过程与栈帧.md) · Ch4 教学子集：[Y86-64](../../chapter-04-processor-architecture/notes/section-4.1-Y86-64-ISA.md)（简化版，非另一套生产 ABI）

---

### x86-64 寄存器分类统计

#### 一、通用整数寄存器（**16** · CSAPP 重点）

运算、传参、栈、指针全靠这组；每个 **64 bit**，可拆 `%rax` / `%eax` / `%ax` / `%al`。

| 组 | 寄存器 |
|----|--------|
| 基础 8（兼容 IA-32 名） | `%rax %rbx %rcx %rdx %rsi %rdi %rbp %rsp` |
| 扩展 8 | `%r8 %r9 %r10 %r11 %r12 %r13 %r14 %r15` |

**ABI 记忆：**

| 角色 | 寄存器 |
|------|--------|
| **caller-saved** | `%rax,%rcx,%rdx,%rsi,%rdi,%r8–%r11` |
| **callee-saved** | `%rbx,%rbp,%r12–%r15` |
| 专用 | `%rsp` 栈顶；`%rbp` 常作帧基址（可省略） |

→ 传参顺序与全景：[§3.7](./section-3.7-过程与栈帧.md) · [ABI §6](../../chapter-02-representing-information/notes/section-2.1.2-abi-application-binary-interface.md#6-x86-64-system-v-abi-全景linux--除类型占几字节以外)

#### 二、浮点 / SIMD（**16 × XMM**）

| | |
|--|--|
| 名 | `%xmm0`–`%xmm15`（各 **128 bit**） |
| 扩展 | AVX → `%ymm*`（256）；AVX-512 → `%zmm*`（512） |
| 用途 | `float`/`double`、向量；传参/返回与 GPR **隔离** |

→ [§3.11](./section-3.11-浮点代码.md)

#### 三、段寄存器（6 · 用户态习题几乎不用）

`%cs %ds %es %fs %gs %ss` — 现代平坦模型下少碰；Linux **TLS** 等会用到 `%fs`/`%gs`。

#### 四、控制 / 系统（用户程序不直接乱写）

| | 作用 |
|--|------|
| **`%rip`** | 下一条指令地址（指令指针） |
| **`%rflags`** | ZF / CF / SF / OF… |
| `CR0`–`CR8`、MSR… | 特权 / 模式；内核与虚拟化才管 |

#### 分场景怎么数？

| 场景 | 答 |
|------|----|
| CSAPP / 用户态整数汇编 / 传参 | **16 个 GPR（`rax`–`r15`）** |
| 加上浮点编程 | 16 GPR + 16 XMM ≈ **32 个可编程槽** |
| 含段/控制/系统 | 更多 — **日常不用背全表** |

**vs ARM32：** 通用整数也是 **16**（`r0`–`r15`），**无** `al/ax` 拆分别名；浮点走 **VFP `s*`/`d*`**，不是 XMM。→ [Smith §2.2](../../../19-ARM64-Architecture/arm32-smith-assembly/chapter-02-programmers-model/notes/section-2-2-data-types.md)

---

### 3.3 数据格式（x86-64 · CSAPP 原文表）

| C 类型 | 字节 | 汇编后缀 | 寄存器片段 | 场景 |
|--------|------|----------|------------|------|
| `char` | 1 | **b** (byte) | `%al`、`%bl`… 低 8 位 | 字符、8 位整数 |
| `short` | 2 | **w** (word) | `%ax`、`%bx`… 低 16 位 | 16 位短整型 |
| `int` | 4 | **l** (long word) | `%eax`、`%ebx`… 低 32 位 | 32 位标准整型 |
| `long` / 指针 | 8 | **q** (quad word) | `%rax`、`%rdi`、`%rbp`… 完整 64 位 | 长整数、**所有地址** |

**CSAPP 关键规则：**

1. GPR 硬件 **64 bit（8B）**，可分段摸低 32/16/8 位（ARM32 **无** 此别名）。  
2. **写 `%eax` → 自动清零 `%rax` 高 32 位**（AMD64 硬件规定）。  
3. **指针一律 8B** → 全局/栈/参数地址常用 **`q`**（`movq`、`leaq`…）。  
4. 宽度靠 **同一助记符 + 后缀**：`movb` / `movl` / `movq` — 不是换指令名。

```asm
movb $1, %al      # 1B char
movl $1, %eax     # 4B int；高 32 位清零
movq $1, %rax     # 8B long/指针
movl $10, %eax
movq (%rax), %rdi
```

#### 对标 ARM32（同一套 C 宽度，不同写法）

| C（ARM32 ILP32） | 字节 | x86 后缀 | ARM32 |
|------------------|------|----------|-------|
| `char` | 1 | b | `ldrb`/`ldrsb` · `strb` |
| `short` | 2 | w | `ldrh`/`ldrsh` · `strh` |
| `int` / 指针 | 4 | l | `ldr` · `str` |
| `long long` | 8 | q | `ldrd` · `strd`（两寄存器） |

- ARM32：**无** `%al`/`%eax` 分段别名；`rN` 始终整 32 位。  
- 寻址：`$imm`→`#imm`，`(%reg)`→`[reg]`，`8(%rbp)`→`[r11,#8]`。  
- 详表 + 例子 → [Smith §2.2 对标](../../../19-ARM64-Architecture/arm32-smith-assembly/chapter-02-programmers-model/notes/section-2-2-data-types.md)

---

#### 通用寄存器一次能存多少？（「几位 CPU」判定）

**判定标准：** **通用寄存器位宽 ≈ CPU 通用整数运算单次处理宽度** → 口语里的「32 位 / 64 位 CPU」。

| 架构 | 通用寄存器位宽 | 单次标准运算块 | 单寄存器最大比特 |
|------|----------------|----------------|------------------|
| **ARM32（AArch32）** | 32 | 32 | 32 |
| **x86-32（IA-32）** | 32 | 32 | 32 |
| **x86-64** | 64 | 64 | 64 |
| **ARM64（AArch64）** | 64 | 64 | 64 |

**x86-64 同一物理槽的分段视图：**

| 视图 | 宽度 | 说明 |
|------|------|------|
| `%rax` | 64 bit（8B） | 完整寄存器；`movq` |
| `%eax` | 低 32 bit（4B） | `movl`；**写 eax 常清零高 32 位** |
| `%ax` | 低 16 bit（2B） | `movw` |
| `%al` | 低 8 bit（1B） | `movb` |

**ARM32 对照：** `r0`–`r15` 固定 32 bit；**64 位大数** 要 **两寄存器拼接**（低/`r0` + 高/`r1`）。AArch64 用完整 `xN`（或半宽 `wN`）。

- **内存永远按字节编址**（每地址 1 字节）— 「32/64 位」不是「内存格子变宽」。→ [Smith §2.2](../../../19-ARM64-Architecture/arm32-smith-assembly/chapter-02-programmers-model/notes/section-2-2-data-types.md)
- → [19-ARM64](../../../19-ARM64-Architecture/) · AAPCS：[§13.5](../../../19-ARM64-Architecture/arm32-smith-assembly/chapter-13-subroutines-stacks/notes/section-13-5-apcs.md)

---

### 3.4.1 操作数指示符（gas / AT&T · CSAPP 标准五种）

| 类型 | 写法 | 含义 | 例 |
|------|------|------|----|
| **立即数** | `$常数` | 编译期常量，只读 | `movl $10, %eax` |
| **寄存器** | `%名` | 寄存器里的值（最快） | `%rax` |
| **绝对地址** | 裸地址 | 固定内存址（现代少见；多用 RIP 相对） | `0x601000` |
| **间接** | `(%reg)` | ≡ C `*p` | `movq (%rax), %rdi` |
| **基址+偏移** | `imm(%reg)` | ≡ `*(base+imm)` — **栈最常用** | `8(%rbp)` |

再往上（热路径）：变址 `(%rax,%rcx,4)` = `rax + rcx*4` — 数组 `A[i]`。

**栈口语：** `%rbp` 常作帧基址；`8(%rbp)` 等取 **更高地址**（返回地址/保存的 rbp/参数区，视帧布局而定）；负偏移多取局部变量。`%rsp` 始终是 **栈顶**；栈向 **低地址** 增长。

```asm
addq 8(%rbp), %rax   # CISC：内存可直接作算术源 — ARM 禁止，须先 ldr
```

**vs ARM32：** 偏移在括号前 `8(%rbp)` ↔ ARM `[r11, #8]`；宽度用后缀 ↔ `ldrb/ldrh/ldr`。

---

### 和 ABI / CSAPP 考点串联

| 条款 | 在本节约等于 |
|------|----------------|
| `int` 4B、指针 8B（LP64） | 后缀 **`l` / `q`** |
| 前 6 整型/指针参数 | `%rdi, %rsi, %rdx, %rcx, %r8, %r9`（ARM32 是 `r0`–`r3`） |
| 栈局部 / 多余参数 / 返回地址 | `n(%rsp)` / `n(%rbp)` |
| 内存参与运算 | x86 **允许** `addq 8(%rbp), %rax`；ARM **Load/Store 强制分步** |
| 浮点 | `%xmm0`–`%xmm15`（与 GPR 分离；→ [§3.11](./section-3.11-浮点代码.md)） |
| 虚拟地址 | 典型 **48-bit canonical** 用户空间（远大于 ARM32 的 4GB 上限；≠ 满 64 bit） |

不遵守宽度或偏移 → 读错字段、栈错乱。手写 asm / 读 `objdump`：**先对后缀，再对寻址**。

---

### 3.4.2–3.4.3 数据传送指令

- `mov` — 源→目的（**不能** mem→mem）
- `movz` / `movs` — 零扩展 / 符号扩展加载较小类型
- `lea` — **只算地址、不访存**（也常用于 `x + k*scale` 快速算术）

### 3.4.4 压入和弹出栈数据

- **栈向低地址增长** — `%rsp` 指向栈顶
- `pushq %rax`：`rsp -= 8`，写入 `(%rsp)`
- `popq %rax`：读出，`rsp += 8`
- `call` = push 返回地址 + jump；`ret` = pop 到 PC

**HFT：** 深调用栈、大栈帧 → cache miss、页 fault；热路径 **内联、尾调用、少 alloca**。

---

### 口述巩固 · 自测

1. CSAPP 全书默认哪套 ISA/ABI？和 ARM32 汇编能混用吗？  
2. x86-64 **通用整数寄存器几个**？caller/callee-saved 怎么分？浮点另有几组 XMM？  
3. `int` / 指针后缀？前 6 个参数寄存器？（ARM32 指针几字节？）  
4. `$0x4` vs `0x4`？`(%rax)` / `8(%rbp)` 各等价什么？  
5. 为何写 `%eax` 会影响 `%rax` 高半？ARM32 有 `%al` 这类别名吗？  
6. `addq 8(%rbp), %rax` 在 ARM 上为何不能一条指令做完？  
7. Ch4 的 Y86-64 和本章真 x86-64 什么关系？

---

← [本章导读](../README.md) · [§3.1–3.2](./section-3.1-3.2-历史观点与程序编码.md) · [§3.7 过程与栈帧](./section-3.7-过程与栈帧.md)
