# Item 1 · 04 枚举与 ADT

← [Item 1 目录](./README.md)

## 定义

**枚举 = ADT（代数数据类型）**——一个值只能是**若干互斥变体之一**，变体可携带数据。

**本质**：C 的 tag enum + **类型安全** tagged union。

## 积类型 vs 和类型（ADT 的两根柱子）

Rust 里复杂数据结构，核心靠这两类 **组合** 出来，没有第三种「基础 ADT 分类」：

| 中文 | 英文 | Rust 典型写法 | 含义 |
|------|------|---------------|------|
| **积类型** | product | `struct`、`tuple`、`(A, B)` | **同时**拥有若干部分（「且」） |
| **和类型** | sum | `enum` | **要么**这一种、**要么**那一种（「或」） |

特殊：`()` **单元类型** = **零元积类型**（没有任何字段的积），唯一值 `()`，表示「无组成部分 / 无有用值」。见 [02-scalar-types.md](./02-scalar-types.md)。

除此之外没有别的 ADT 基类，但可以**无限嵌套组合**：

- enum 变体里带 struct 字段（和 + 积）；
- struct 里嵌 enum（积 + 和）；
- 多层 enum / struct 描述复杂业务状态。

### HFT：订单状态 = 和类型包积类型

用 **enum（和）** 包住 **struct（积）**，每个状态只带**该状态下合法**的字段，避免「已撤销订单还挂着成交价」：

```rust
struct FillInfo {
    price: f64,
    qty: u32,
}

enum OrderState {
    Pending { symbol: String, qty: u32 },
    Filled(FillInfo),
    Cancelled, // 无成交数据
}
```

`Cancelled` 变体**不可能**携带 `FillInfo`——无效状态在类型层面写不出来（Item 1 核心主张）。

## 三种变体形式

| 变体形式 | 示例 |
|----------|------|
| 单元变体 | `Quit` |
| 元组变体 | `Write(String)` |
| 结构体变体 | `Move { x: i32, y: i32 }` |

```rust
enum Message {
    Quit,
    Write(String),
    Move { x: i32, y: i32 },
}
```

## 与 `match`

- 必须**穷尽**所有变体，否则不编译。
- 公开 `enum` 加变体 = **破坏性变更** → 见 [07-pitfalls.md](./07-pitfalls.md)

## 相关

- 聚合（积类型）→ [03-aggregate-types.md](./03-aggregate-types.md)
- 内建 enum → [05-option-result.md](./05-option-result.md)
- 重构案例 → [06-design-patterns.md](./06-design-patterns.md)
