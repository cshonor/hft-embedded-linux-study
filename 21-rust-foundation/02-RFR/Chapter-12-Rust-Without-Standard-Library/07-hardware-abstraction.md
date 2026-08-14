# 5. Misuse-Resistant Hardware Abstraction（防误用硬件抽象）

> [← 章索引](./README.md)

用 **ZST + `PhantomData`** 或 **消耗 `self` 的状态转移** 使非法硬件组合**不可构造**。

## 例

继电器 / 时钟域 **互斥配置**：

```text
Pair<StateA, StateB> — 切换仅 via 消耗旧状态 → 返回新状态
```

## 零成本

理想下无额外字段 — 错误在**编译期**拒绝。

→ [第 3 章 typestate](../Chapter-03-Designing-Interfaces/10-type-system-guidance.md) · [第 1 章 · 无效状态不可表达](../../01-ER/Chapter-01-Types/Item-01-express-data-structures/06-design-patterns.md)
