# Item 3 · 核心知识点

← [Item 3 目录](./README.md)

| 概念 | 说明 |
|------|------|
| **`Option<T>`** | 值存在 `Some(T)` 或缺失 `None` |
| **`Result<T, E>`** | 成功 `Ok(T)` 或失败 `Err(E)` |
| **转换方法（Transforms / Adaptors）** | `map`、`map_err`、`and_then`、`unwrap_or`、`as_ref` 等，链式处理内部值而少写分支 |
| **`?` 运算符** | 成功则剥出 `Ok`；失败则**提前返回**，并常通过 `From` 做错误类型转换 |
| **解包（Unwrapping）** | `.unwrap()` / `.expect()`：失败即 **`panic!`**（鸵鸟策略，放弃恢复） |

---
