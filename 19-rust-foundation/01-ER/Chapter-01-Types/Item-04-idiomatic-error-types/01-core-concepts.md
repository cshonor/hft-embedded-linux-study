# Item 4 · 核心知识点

← [Item 4 目录](./README.md)

### `std::error::Error` trait

- `Result<T, E>` 的 **`E` 不强制**实现 `Error`，但实现它是与生态对齐的**惯例**。
- 实现 `Error` 前必须已有：
  - **`Display`**（`{}`，给用户看）
  - **`Debug`**（`{:?}`，给开发者调试）

### `source()` 方法

- 默认返回 `None`；可重写以暴露**底层原因**（**「父错误指针」** → `Some(&底层)`）：
  - `fn source(&self) -> Option<&(dyn Error + 'static)>`
- 量化 **`StrategyError` + CSV `io::Error`** 示例与 **`thiserror` 自动挂指针** → [RFR Ch04 错误链](../../../02-RFR/Chapter-04-Error-Handling/01-error-source-chain.md#心智模型source--父错误指针)

### 孤儿规则（Orphan Rule）

- 只有「trait 或类型至少一方定义在当前 crate」才能 `impl`。
- **不能**写 `impl Error for String`（`String` 与 `Error` 都在标准库）。

### 谁实现了 `Error`？

| 来源 | 例子 |
|------|------|
| **标准库** | `std::io::Error`、`ParseIntError`、`FromUtf8Error` 等 |
| **手写 `impl`** | 见 [04-examples.md](./04-examples.md) 里的 `MyError` enum |
| **`thiserror` 派生** | `#[derive(Error)]` 自动生成 `impl Error` + 把 `#[error("...")]` 绑到 `Display` |

`thiserror` 适合**库**：省掉重复的 `impl Display` / `impl Error` / `source()` 样板，让你专注错误语义本身。应用层汇总多库异构错误时更常用 `anyhow`（见 [03-key-takeaways.md](./03-key-takeaways.md)）。

---
