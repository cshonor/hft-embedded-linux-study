# 第 5 章 · 中间表示 · §5 SSA（静态单赋值）

← [本章目录](./README.md) · 上一节：[04-linear-ir.md](./04-linear-ir.md) · 下一节：[06-names-and-memory-models.md](./06-names-and-memory-models.md)

---

**SSA（Static Single-Assignment Form）** — 现代优化编译器**极重要**的高级 IR 形态。

→ ch9～10 深度使用 · [04 LLVM IR（SSA）](../../../04_Learn-LLVM-17/README.md)

---

## 核心规则

> 程序中**每个「变量」只被赋值一次**。

原代码多次赋值 `x` → SSA 重命名为 `x1`, `x2`, `x3`… 各名**唯一 def**。

---

## φ 函数（Phi, φ-function）

**控制流合并**处（如 `if-else` 汇合），不知运行时取哪条路径的值：

```text
if (c)  x1 = a;  else  x2 = b;
        x3 = φ(x1, x2)    ← 按实际到达的前驱块选 x1 或 x2
```

| 作用 | 保持 SSA 不变式 + 表达「合流点选值」 |

---

## 为何简化数据流分析

| 无 SSA | 有 SSA |
|--------|--------|
| 一变量多 def，需复杂 reaching-defs | **每名一次 def** → use-def 链**直接** |
| 常量传播、DCE、GVN 等更繁琐 | ch8～10 标量优化**标准底座** |

**Rust / LLVM**：优化 Pass 几乎都在 **LLVM IR（SSA）** 上跑。

---

## 与其他 IR 关系

```text
三地址码 / CFG  →  （插入 φ、重命名）  →  SSA  →  优化  →  退出 SSA（寄存器分配前）
```

---

## 自测

- [ ] SSA 一条规则
- [ ] φ 在什么位置出现、解决什么问题
- [ ] 为何 LLVM 选 SSA 做优化 IR
