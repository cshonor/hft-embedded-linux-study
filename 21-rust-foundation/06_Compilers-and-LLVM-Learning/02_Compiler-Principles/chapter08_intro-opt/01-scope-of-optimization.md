# 第 8 章 · 代码优化概述 · §1 优化的作用域

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-redundant-expressions.md](./02-redundant-expressions.md)

---

编译器**无法**每次把整个程序载入内存做单一巨型分析 — 必须按**分析/变换的范围**划分策略。

---

## 五层作用域（由小到大）

| 层次 | 英文 | 范围 | 典型 Pass 例子 |
|------|------|------|----------------|
| **局部** | Local | **单个基本块**内 | 块内 DAG、局部 VN |
| **超局部** | Superlocal | **一条**控制流路径上若干块 | 路径上 VN |
| **区域** | Regional | 有结构的**区域**（常 = **循环**） | 循环不变式外提、LICM |
| **全局** | Global | **整函数 / 过程** | 可用表达式、活跃变量 |
| **全过程** | Whole-program | **跨过程**边界 | 内联、IPO、LTO |

```text
Local ⊂ Superlocal ⊂ Regional ⊂ Global ⊂ Whole-program
         （分析代价 ↑ · 优化机会 ↑）
```

---

## 与 ch5 CFG

- **基本块** = 局部优化的自然边界 → [ch5 §3](../chapter05_ir/03-graphical-ir.md)
- **循环** = 区域方法的主战场（归纳变量、向量化）
- **函数** = 全局数据流的编译单元（LLVM 一函数一块 IR 常见）

---

## 与 Rust / LLVM

| 实践 | 作用域 |
|------|--------|
| **函数内 `-O2` Pass 链** | Global（单 crate 内函数） |
| **`#[inline]` / LTO** | Whole-program |
| **基本块 peephole** | Local |

**HFT**：关注 **Global + Whole-program** 能否消 call、消冗余 load — 但 LTO 增编译时间。

---

## Trade-off（ch1 再现）

| 更大作用域 | 收益 | 成本 |
|------------|------|------|
| ✓ | 更多冗余可消、更多常量可折叠 | 分析慢、内存大、实现复杂 |

优化器 = **Pass 管线**：先 cheap local，再 expensive global。
