# 1.3 Special Error Cases（特殊错误情形）

> 所属：**Representing Errors** · [← 章索引](./README.md)

## `Result<T, ()>` vs `Option<T>`

**不等价**：

- **`Err(())`** — 强调「操作失败」，常隐含需处理 / 重试。
- **`None`** — 「此处无值」，**未必**是异常。

简化为 `Option` 前应优先服从**语义**，而非省字数。

## Never 类型 `!`

- 表示**不可能产生的值**。
- `Result<T, !>` → 编译器知 **`Err` 不可达**（与收窄优化相关）。

## `std::thread::Result`

Join 错误为 **`Box<dyn Any + Send + 'static>`**，不是 `dyn Error`：

- `panic!` 可携带**任意类型** payload，未必实现 `Error`。

Book → [9.2 Result](../../00-Book/09-error-handling/9.2-Result-与可恢复的错误.md) · [ER Item 01 · Option/Result](../../01-ER/Chapter-01-Types/Item-01-express-data-structures/05-option-result.md)
