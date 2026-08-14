# Item 4 · 易错细节

← [Item 4 目录](./README.md)

| 误区 | 说明 |
|------|------|
| **`type MyError = String`** | 类型别名**不是新类型**，仍不能 `impl Error for MyError` |
| **`no_std`** | 稳定版上 `Error` 在 `std`，嵌入式可能用不了；关注 `core::error::Error` 演进 |
| **手写 `From<E> for Wrapped` 泛型** | 易与 `impl<T> From<T> for T` 冲突 → **E0119**；应用层用 **anyhow** |
| **库里的 `unwrap` / `String` 错误** | 公共 API 应给调用方可恢复、可匹配的错误类型 |

---
