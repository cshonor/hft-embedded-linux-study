# 第 7 章 · 代码形态 · §2 操作符的底层转换

← [本章目录](./README.md) · 上一节：[01-storage-locations.md](./01-storage-locations.md) · 下一节：[03-data-structures.md](./03-data-structures.md)

---

将源语言**高级操作符**映射为 IR / 硬件指令 — 多种形态可选。

---

## 算术操作符

| 话题 | 代码形态考量 |
|------|--------------|
| **寄存器压力** | 长表达式：拆成多条三地址 vs 保留在栈 |
| **混合类型** | `int + float` — 插入 **convert** 再运算 |
| **硬件特性** | 乘法比加法慢、移位代替 `*2^n`、融合乘加（FMA） |

**Rust**：类型 promotion 在类型检查阶段定；LLVM 选具体 x86 指令。

**HFT**：避免隐式分配/除法；用位运算、查表 — 形态选择在源码 + IR 层配合。

---

## 布尔与关系操作符

两种常见**表示策略**（可混合）：

| 策略 | 形态 | 适用 |
|------|------|------|
| **数值表示** | `true=1, false=0`；`cmp` + `setcc` + `and/or` | 表达式内布尔运算 |
| **控制流表示** | 条件分支到不同块，**不**物化 0/1 | 短路 `&&` `\|\|`、if 条件 |

```text
// 控制流形态（概念）
if (a < b)  goto Ltrue;  else goto Lfalse;
Ltrue:  res = …;
```

| 硬件 | 选择 |
|------|------|
| **条件码（flags）** | x86 `cmp` + `jl` — 少生成显式布尔值 |
| **谓词 / select** | IR `select` — SIMD / 无分支 |

**clox**：比较 + `OP_JUMP_IF_FALSE` — 控制流形态 → [ch17](../../../01_Crafting-Interpreters/part03_clox/chapter17_compiling-expressions/README.md)

---

## 与优化

- **数值布尔** + 纯 SSA → 易做常量传播。
- **控制流布尔** → 易做分支消除、布局优化（ch7 §4）。

形态选择 = 给 ch8 **哪种优化留门**。
