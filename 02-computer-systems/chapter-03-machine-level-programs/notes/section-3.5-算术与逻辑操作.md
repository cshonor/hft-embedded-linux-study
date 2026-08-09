## 3.5 算术和逻辑操作

### 3.5.1 加载有效地址 `lea`

```asm
leaq 8(%rdi,%rsi,4), %rax   # rax = rdi + rsi*4 + 8
```

- 不访问内存 — 编译器常用 `lea` 做 **乘法加法的 cheap 形式**

### 3.5.2 一元和二元操作

- `inc/dec/neg/not` — 一元
- `add/sub/imul` — 二元；`imul` 有单操作数形式 → 宽乘
- `xor` — 清零寄存器惯用法：`xor %eax, %eax`

### 3.5.3 移位

- `sal/sar` — 左移 / **算术**右移（有符号）
- `shl/shr` — 逻辑移位
- **移位量** 常为立即数或 `%cl`（固定约定）

### 3.5.4–3.5.5 讨论与特殊算术

- **溢出：** 无符号用 CF；有符号乘除用 `imul`/`idiv` 扩展 `cqto`（rax→rdx:rax）
- `mul`/`div` — 无符号；`div` 慢，热路径避免

**HFT / 优化（强度削减）：**

- `a*4` / `a*8` 等：好的编译器常译成 **`shl` / `lea`**，而不是慢 `imul` — 源码是乘，机器不一定是乘；用 `gcc -S` 核实  
- 除法常比移位慢一个数量级 — 编译器 strength reduction（→ [Ch 5](../../chapter-05-optimizing-performance/)）  
- `perf` 里热点若大量 `idiv` — 考虑倒数乘法或移位  

### 自测题

<details>
<summary>1. `imul` 的两种形式有什么区别？</summary>

1. **双操作数** `imul %rax, %rbx`：`rbx = rbx * rax`，只保留低 64 位（溢出部分丢失）。用于普通乘法。
2. **单操作数** `imul %rax`：`RDX:RAX = RAX * operand`，128 位结果（高 64 位在 RDX）。用于需要完整乘积的场合。

HFT 注意：双操作数 `imul` 不检测溢出（静默截断），需要 `__builtin_mul_overflow` 检查。

</details>

<details>
<summary>2. `cqto` 指令的作用是什么？为什么除法前需要它？</summary>

`cqto`（CQO）= Convert Quadword to Octword：把 RAX 的符号位扩展到 RDX:RAX（128 位）。`idiv` 指令用 RDX:RAX ÷ 操作数，所以除法前必须 `cqto` 设置 RDX。否则 RDX 中的垃圾值会导致除法结果错误。这是有符号除法的标准 preamble：`cqto; idivq %rcx`。

</details>


---

← [本章导读](../README.md) · [§3.4.4 ←](./section-3.4.4-压栈与弹栈.md) · [§3.6 →](./section-3.6-控制流.md)
