# Item 1 · 07 易错细节

← [Item 1 目录](./README.md)

## `Option` 与「空集合」

核心：**`None` = 不存在；`Some(vec![])` = 存在但空** —— 详见 [05-option-result.md](./05-option-result.md) 三层分工。

| 表达 | 何时用 |
|------|--------|
| `Vec<T>` / `String` 为空 | 字段**必定出现**，空 = 没内容 → `Result<Vec<T>, E>` 即可 |
| `Option<Vec<T>>` | 必须区分 **字段不存在**（`None`）vs **字段存在但空**（`Some(vec![])`) |
| `Result::Err` | 断网、超时、无权限 —— **不是** `None` |

常见误区：把 `None` 当空集合；把 `Some` 理解成「必须有元素」。

---

## 公开 `enum` 加变体 = 破坏性变更

- 外部 `match` 因**穷尽性**未覆盖新变体 → **编译失败**。
- 仅需 C 风格常量扩展时，可考虑 `#[non_exhaustive]`（调用方须留 `_` 等兜底分支）。

## 相关

- enum 基础 → [04-enum-adt.md](./04-enum-adt.md)
- Option 原则 → [05-option-result.md](./05-option-result.md)
