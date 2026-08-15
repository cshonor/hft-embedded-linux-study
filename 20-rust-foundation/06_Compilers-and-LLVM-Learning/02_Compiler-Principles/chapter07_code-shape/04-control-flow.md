# 第 7 章 · 代码形态 · §4 控制流结构

← [本章目录](./README.md) · 上一节：[03-data-structures.md](./03-data-structures.md) · 下一节：[05-calls-and-oop.md](./05-calls-and-oop.md)

---

程序逻辑 → **条件分支 + 无条件跳转** — 影响 CFG 与分支预测。

---

## 条件执行（if-then-else）

| 布局策略 | 说明 |
|----------|------|
| **then/else 块 + 汇合块** | 经典 CFG；SSA 合流点插 **φ** |
| **分支布局** | 把**更常走**的分支放在 fall-through — 减 taken penalty |
| **条件移动（cmov）** | 无分支形态 — 小表达式、避免 mispredict 代价 |

```text
        cond
       /    \
   then      else
       \    /
        merge
```

**clox**：`OP_JUMP` / `OP_JUMP_IF_FALSE` → [ch23](../../../01_Crafting-Interpreters/part03_clox/chapter23_jumping-back-and-forth/README.md)

---

## 循环（while / for）

| 形态 | 要点 |
|------|------|
| **前置/后置条件** | 是否至少执行一次 |
| **归纳变量** | 便于 ch8 **强度削减**、向量化 |
| **出口块** | 与 SSA φ、break/continue 汇合 |

形态应暴露 **loop header / latch / exit** — 供循环优化识别。

---

## switch / case

| 策略 | 适用 |
|------|------|
| **线性 if-else 链** | case 少 |
| **二分搜索** | 有序、中等数量 |
| **跳转表（Jump table）** | 密集整数 case — O(1) 跳转 |

```text
jump_table[discriminant - base]  →  target BB
```

**Rust `match`**：编译器选 tree vs table；**穷尽检查**在 ch4 语义层。

---

## 与 ch5 CFG

控制流 lowering 直接**塑造 CFG** — 决定 ch8 数据流分析域与 ch12 **指令调度** 的基本块边界。

---

## HFT 提示

- 热路径：**可预测分支**、少间接跳转。
- `match` 跳转表 vs 链 — 常数时间 dispatch 形态可选。
