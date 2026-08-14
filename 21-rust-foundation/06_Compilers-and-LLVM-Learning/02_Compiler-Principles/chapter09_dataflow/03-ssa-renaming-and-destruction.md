# 第 9 章 · 数据流分析 · §3 SSA 重命名与代码复原

← [本章目录](./README.md) · 上一节：[02-dominance-and-phi.md](./02-dominance-and-phi.md) · 下一节：[04-advanced-topics.md](./04-advanced-topics.md)

---

## 重命名（Renaming）

φ 已插入后，系统遍历 **支配树**，将原变量改为 **x1, x2, x3…** — 保证 **每条 SSA 名仅一次 def**。

| 机制 | 说明 |
|------|------|
| **栈 per 原变量** | 跟踪当前版本号 |
| **遍历支配树** | 进入块：push 新版本；离开：pop |
| **use 替换** | 读操作使用栈顶当前名 |
| **φ 参数** | 按前驱块对应版本填 φ Operands |

```text
// 概念
x = …      →  x1 = …
x = …      →  x2 = …
… use x …  →  … use x2 …
```

**结果**：满足 SSA 严格定义 — 优化 Pass 可依赖 **单次赋值**。

---

## 代码复原（SSA Destruction / Out of SSA）

优化完成后，目标机通常**没有 φ 指令** — 必须**降回**普通代码。

| 策略 | 说明 |
|------|------|
| **φ 拆成拷贝** | 在每个前驱块末尾插入 `x = xi` |
| **并行拷贝问题** | 多 φ 同时更新 — 需 **swap/临时** 序列 |
| **与 regalloc 衔接** | ch13 可在 SSA 退出前后分配；许多编译器 **先破坏 SSA 再 regalloc** 或 **在 SSA 上 regalloc 再插入 moves** |

**LLVM**：后端 Legalize 后 φ 变为 **mov**；或 **寄存器分配器** 处理 φ 为并行 copy。

**正确性**：破坏 SSA 不得改变语义 — 仍受 [ch1 正确性](../chapter01_overview/02-two-fundamental-principles.md) 约束。

---

## 完整 SSA 管线（复习）

```text
普通 IR  →  [φ 放置]  →  [重命名]  →  SSA IR
                ↓
           ch10 优化 Pass
                ↓
           [SSA 销毁]  →  可 codegen IR
```

---

## 与 Rust

- **MIR** 接近 SSA 风格；→ LLVM IR（显式 `phi`）。
- 读 `opt`- 后 IR diff 时：φ / 重命名版本即本章产物。

---

## 自测

- [ ] 重命名为何沿支配树 walk
- [ ] φ 销毁为何要处理「并行拷贝」
