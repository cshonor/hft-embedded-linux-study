## §18.5 练习题

> **Ch 18 · C 与汇编混合编程** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 练习方向（原书典型）

| 类型 | 练什么 |
|------|--------|
| **Inline** | 读 **Q 标志** 或单条 **SSAT** |
| **Embedded** | **`asm_memcpy`** / **`asm_strcpy`** — 完整 AAPCS |
| **C→asm** | `main` 调 asm 求 **max(a,b)** |
| **asm→C** | asm 循环里 **`BL printf`**（Semihosting 或 UART） |
| **栈参** | C 函数 **5 个 int 参数** — asm 侧 **`PUSH` 第 5 个** |
| **VCVT** | 封装 **`fixed_to_float`** 供 C 测试 |

---

### 自测 Checklist

- [ ] **Inline vs Embedded** 适用场景与限制表  
- [ ] Inline **禁止** 哪些（BX、改 PC、伪指令…）  
- [ ] Embedded **谁写返回** · **谁保证 AAPCS**  
- [ ] **前导零** 在 asm vs C 的差异  
- [ ] **C 调 asm** / **asm 调 C** 参数寄存器  
- [ ] **`extern "C"`** 何时需要  

---

### 常见错误

| 错误 | 后果 |
|------|------|
| Embedded **忘记 POP/BX lr** | 跑飞 |
| 用 **r4** 不保存 | C 变量被破坏 |
| C++ 无 **`extern "C"`** | 链接 **undefined reference** |
| Inline 里 **改 SP** | 帧损坏 |
| **栈不对齐** 调 C | HardFault |

---

### 延伸

| 方向 | 参考 |
|------|------|
| **奔跑吧 Ch10** | **GCC `__asm__`** 约束与 clobber |
| **Linux kernel** | **`asmlinkage`** · **`ENTRY()`** 宏 |
| **Ch16** | **`volatile` 指针** 访问 MMIO |

---

### 可复述要点

1. 精读章 — 能 **写最小 embedded 函数 + C main 调用** 即达标。  
2. **AAPCS 表** 与 **Ch13** 闭卷一致。  
3. Smith **正文混编收官** — 后续附录可跳过，转 **奔跑吧/GCC/模块 20**。
