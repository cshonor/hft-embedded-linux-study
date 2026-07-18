## 3.3 数据格式（x86-64）

> **CSAPP 全书基准：** **x86-64 + Linux gas（AT&T）+ System V ABI**；**HFT 只练 AT&T**。  
> 与 ARM32 对照 → [Smith §2.2](../../../19-ARM64-Architecture/arm32-smith-assembly/chapter-02-programmers-model/notes/section-2-2-data-types.md)

> [章导读](../README.md) · 上节 [§3.2.3](./section-3.2.3-AT&T汇编语法.md) · 下节 [§3.4.1 操作数](./section-3.4.1-操作数指示符.md)  
> ABI：[Ch2 · ABI](../../chapter-02-representing-information/notes/section-2.1.2-abi-application-binary-interface.md) · 传参：[§3.7](./section-3.7-过程与栈帧.md) · Y86：[§4.1](../../chapter-04-processor-architecture/notes/section-4.1-Y86-64-ISA.md)

---

### x86-64 寄存器分类统计

#### 一、通用整数寄存器（**16** · CSAPP 重点）

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

→ [§3.7](./section-3.7-过程与栈帧.md) · [ABI §6](../../chapter-02-representing-information/notes/section-2.1.2-abi-application-binary-interface.md#6-x86-64-system-v-abi-全景linux--除类型占几字节以外)

#### 二、浮点 / SIMD（**16 × XMM**）

| | |
|--|--|
| 名 | `%xmm0`–`%xmm15`（各 **128 bit**） |
| 扩展 | AVX → `%ymm*`（256）；AVX-512 → `%zmm*`（512） |
| 用途 | `float`/`double`、向量；传参/返回与 GPR **隔离** |

→ [§3.11](./section-3.11-浮点代码.md)

#### 三、段寄存器（6 · 用户态习题几乎不用）

`%cs %ds %es %fs %gs %ss` — Linux **TLS** 等会用到 `%fs`/`%gs`。

#### 四、控制 / 系统（用户程序不直接乱写）

| | 作用 |
|--|------|
| **`%rip`** | 下一条指令地址 |
| **`%rflags`** | ZF / CF / SF / OF… |
| `CR0`–`CR8`、MSR… | 特权 / 模式 |

#### 分场景怎么数？

| 场景 | 答 |
|------|----|
| CSAPP / 用户态整数汇编 / 传参 | **16 个 GPR（`rax`–`r15`）** |
| 加上浮点编程 | 16 GPR + 16 XMM ≈ **32 个可编程槽** |
| 含段/控制/系统 | 更多 — **日常不用背全表** |

**vs ARM32：** 通用也是 **16**（`r0`–`r15`），**无** `al/ax` 拆分别名；浮点走 **VFP**。→ [Smith §2.2](../../../19-ARM64-Architecture/arm32-smith-assembly/chapter-02-programmers-model/notes/section-2-2-data-types.md)

---

### CSAPP 原文：C 类型 ↔ 后缀 ↔ 寄存器片段

| C 类型 | 字节 | 汇编后缀 | 寄存器片段 | 场景 |
|--------|------|----------|------------|------|
| `char` | 1 | **b** | `%al`… | 字符、8 位整数 |
| `short` | 2 | **w** | `%ax`… | 16 位短整型 |
| `int` | 4 | **l** | `%eax`… | 32 位标准整型 |
| `long` / 指针 | 8 | **q** | `%rax`、`%rdi`… | 长整数、**所有地址** |

**关键规则：**

1. GPR **64 bit**，可分段摸低 32/16/8 位（ARM32 **无** 此别名）。  
2. **写 `%eax` → 清零 `%rax` 高 32 位**（AMD64）。  
3. **指针一律 8B** → 常用 **`q`**。  
4. 宽度靠 **同一助记符 + 后缀**：`movb` / `movl` / `movq`。

```asm
movb $1, %al
movl $1, %eax     # 高 32 位清零
movq $1, %rax
movq (%rax), %rdi
```

#### 对标 ARM32

| C（ARM32 ILP32） | 字节 | x86 后缀 | ARM32 |
|------------------|------|----------|-------|
| `char` | 1 | b | `ldrb`/`ldrsb` · `strb` |
| `short` | 2 | w | `ldrh`/`ldrsh` · `strh` |
| `int` / 指针 | 4 | l | `ldr` · `str` |
| `long long` | 8 | q | `ldrd` · `strd` |

详表 → [Smith §2.2](../../../19-ARM64-Architecture/arm32-smith-assembly/chapter-02-programmers-model/notes/section-2-2-data-types.md)

#### 「几位 CPU」判定

**通用寄存器位宽 ≈ 通用整数运算单次宽度。**

| 架构 | GPR 位宽 | 单次标准运算块 |
|------|----------|----------------|
| ARM32 / IA-32 | 32 | 32 |
| x86-64 / AArch64 | 64 | 64 |

| 视图 | 宽度 |
|------|------|
| `%rax` | 64 bit |
| `%eax` | 低 32；写时常清高半 |
| `%ax` / `%al` | 16 / 8 |

---

### 口述巩固 · 自测

1. CSAPP 默认哪套 ISA/ABI？和 ARM32 能混用吗？  
2. x86-64 **通用整数寄存器几个**？caller/callee-saved？XMM 几组？  
3. `int` / 指针后缀？写 `%eax` 为何影响 `%rax` 高半？

---

← [本章导读](../README.md) · [§3.2.3 ←](./section-3.2.3-AT&T汇编语法.md) · [§3.4.1 →](./section-3.4.1-操作数指示符.md)
