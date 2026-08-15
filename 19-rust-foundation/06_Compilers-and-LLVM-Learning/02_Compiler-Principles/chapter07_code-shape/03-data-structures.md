# 第 7 章 · 代码形态 · §3 复杂数据结构

← [本章目录](./README.md) · 上一节：[02-translating-operators.md](./02-translating-operators.md) · 下一节：[04-control-flow.md](./04-control-flow.md)

---

高级数据结构 → 显式**地址计算** + **布局** +（可选）**边界检查**。

---

## 数组（Arrays）

| 话题 | 说明 |
|------|------|
| **布局** | **行主序（row-major）** vs **列主序（column-major）** — 影响多维索引公式 |
| **地址计算** | `base + (i * stride + j) * elem_size` — 编译期折叠常量部分 |
| **范围检测** | 运行时 bounds check vs 优化掉（Rust 可 panic / 证明安全后消除） |

```text
// 2D 行主序 a[i,j] 概念
addr = base + (i * col_count + j) * sizeof(T)
```

**性能**：内层循环应沿**连续内存**迭代 — 形态 + 循环嵌套顺序决定 cache 行为（HFT 矩阵运算）。

---

## 字符串

| 话题 | 形态 |
|------|------|
| **长度表示** | 前缀长度 / NUL 终止 / fat pointer（ptr+len） |
| **操作** | 赋值、连接 — 可能触发 **堆分配**（新 buffer） |

**Rust `&str` / `String`**：fat pointer vs owned — 编译器生成不同 copy/alloc 序列。

**clox**：**interned** 字符串 → [ch19 Strings](../../../01_Crafting-Interpreters/part03_clox/chapter19_strings/README.md)

---

## 结构体 / 记录（Structs / Records）

| 话题 | 说明 |
|------|------|
| **字段布局** | 顺序、**对齐（align）**、padding |
| **访问** | `base + field_offset` |
| **Rust** | `repr(C)` / `repr(Rust)` / 手动 `#[repr(packed)]` |

→ RFR [02 layout](../../../02-RFR/Chapter-02-Types/02-layout.md) · FFI [ch6 §4](../chapter06_procedures/04-parameter-passing.md)

---

## 与 IR

- 数组/字段访问常 lowering 为 **GEP（getelementptr）** 类地址运算 — LLVM 风格。
- 错误形态 → 多余 mul/add → 优化难消 → 后端慢。
