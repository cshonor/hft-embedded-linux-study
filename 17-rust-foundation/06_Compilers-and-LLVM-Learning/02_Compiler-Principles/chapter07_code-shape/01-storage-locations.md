# 第 7 章 · 代码形态 · §1 指定存储位置

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-translating-operators.md](./02-translating-operators.md)

---

编译器决定：值在**寄存器**还是**内存** — 直接影响性能与优化。

---

## 尽量放在寄存器

| 目标 | 说明 |
|------|------|
| **避免内存访问** | 寄存器比 L1/L2/DRAM **快几个数量级** |
| **做法** | IR 用**虚拟寄存器**（reg-to-reg 模型）→ ch13 分配物理寄存器 |

→ [ch5 §6 reg-to-reg](../chapter05_ir/06-names-and-memory-models.md)

**HFT / Rust**：热路径变量应能驻留寄存器；`#[inline]` + 简单控制流帮助 LLVM **mem2reg** / SROA。

---

## 歧义值（Ambiguous Values）

**变量的地址可被指针/引用操作** → 编译器无法在编译期确定**别名关系** → 该值**不能**假设只活在寄存器里。

| 情况 | 后果 |
|------|------|
| `&x` 传出、`&mut x`、可能别名 | **强制 spill 到内存** |
| 优化受限 | 不能随意把 `x` 只保留在 `%rax` 而不写回 |

**Rust**：借用分析 **减少**歧义（比 C 强）；`unsafe` / 原始指针再引入别名 → 优化 Pass 保守。

**LLVM**：`load`/`store` + `alias analysis` / `noalias` 元数据。

→ RFR 第 10 章 · Nomicon

---

## 与 ch6 寻址

| 已分配在 | 代码形态 |
|----------|----------|
| AR 槽（栈） | `FP + offset` load/store |
| 全局 | 绝对 / PC-relative |
| 寄存器 | 纯 reg-reg 运算 |

**选型链**：语义 → 是否可寄存器化 → IR 形态 → ch8 优化 → ch13 regalloc。

---

## 自测

- [ ] 为何「可取址」常意味着不能永远驻留寄存器？
- [ ] Rust 相比 C 在「歧义值」上编译器多知道什么？
