# 4.1 Type Modifications（类型演进）

> 所属：**Constrained** · [← 章索引](./README.md)

在 semver 兼容前提下**收紧公开结构**，保留增字段 / 增变体空间。

## 可见性

- 尽量少 **`pub`**；模块内部用 **`pub(crate)`**、`**pub(super)**`。
- 公开 surface 越小，重构越自由 → [ER Item 22](../../01-ER/Chapter-04-Dependencies/Item-22-minimize-visibility/README.md)

## `#[non_exhaustive]`

对用户可见的 **`struct` / `enum`**：

- 外部不能 **穷尽 match** 所有变体/字段。
- 不能在 crate 外用 struct literal **假定全字段**。
- 允许在 minor 版本增字段/变体而不破 semver。

## 密封字段模式

- 私有字段 + 构造函数 / builder，避免外部直接构造破坏不变式。

Book → [7.3 路径与可见性](../../00-Book/07-packages-modules/7.3-路径用于引用模块树中的项.md)
