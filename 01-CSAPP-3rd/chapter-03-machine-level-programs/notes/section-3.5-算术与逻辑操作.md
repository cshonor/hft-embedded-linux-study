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

---

← [本章导读](../README.md) · [§3.4.4 ←](./section-3.4.4-压栈与弹栈.md) · [§3.6 →](./section-3.6-控制流.md)
