# 第 13 章 · 寄存器分配 · §1 核心背景

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-local-regalloc.md](./02-local-regalloc.md)

---

## 内存 vs 寄存器

| 存储 | 典型延迟 | 编译器目标 |
|------|----------|------------|
| **寄存器** | ~1 cycle | 尽量让**活跃值**驻留 |
| **L1/L2/主存** | 高得多 | 少 **load/store** |

**溢出（Spill）**：物理寄存器不够 → **store 到栈槽** / 用时 **load 回来** — 显著性能惩罚，循环内尤甚。

→ HFT 热点：看汇编里是否出现意外 `mov [rsp+…]`。

---

## 分配 vs 赋值（Allocation vs Assignment）

| 术语 | 回答的问题 |
|------|------------|
| **Allocation（分配）** | **哪些值**应争取放在寄存器里（哪些 spill） |
| **Assignment（赋值）** | 选中后放进 **哪一个** 物理寄存器（r0…r(k−1)） |

```text
Allocation:  { v1, v3, v7 } → 寄存器；{ v2 } → 栈
Assignment:  v1→rax, v3→rbx, v7→rcx
```

两步可合并实现，但概念分离有助于理解 **spill 决策** vs **着色编号**。

---

## 寄存器分类（Register Classes）

现代 ISA 有多类寄存器，**分开管理**：

| 类别 | 示例 |
|------|------|
| **通用整数（GPR）** | x86 `rax`–`r15` |
| **浮点 / SIMD** | `xmm`/`ymm` |
| **特殊用途** | 栈指针、帧指针、参数寄存器 |

分配器通常为每类建**独立冲突图** — 整数 live range 不与 XMM 混色。

**Calling convention**：部分寄存器 **caller-saved / callee-saved** — 跨调用边界影响 LIVE 与可分配集合。

→ [ch6 调用约定](../chapter06_procedures/README.md) · RFR [FFI calling conventions](../../../02-RFR/Chapter-11-Foreign-Function-Interfaces/02-calling-conventions.md)

---

## 虚拟寄存器

ch11 指令选择后 IR 仍用 **vreg** — 数量不限；ch13 将其映射到 **preg** 或 **spill slot**。

**LLVM**：`%0`, `%1`… → 物理或 `stack` slot。

---

## 自测

- [ ] spill 为何慢
- [ ] Allocation 与 Assignment 各一句话
- [ ] 为何整数与浮点分开着色
