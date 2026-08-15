# Item 18 · 重点结论

← [Item 18 目录](./README.md)

### 最高准则：优先 `Result`，不是 `panic!`

- 可预期、可恢复的错误 → **`Result<T, E>`** + **`?`**。

### 允许 `panic!` 的场景

| 场景 | 说明 |
|------|------|
| **`main` / 二进制顶层** | 无更上层可推诿 |
| **内部状态损坏** | 非外部非法输入，而是 invariant 已破 |
| **成对 API** | fallible + infallible 并存，如 `from_utf8` / `from_utf8_unchecked` |

### 公共 API 文档

- 可能 panic 的函数 → **`# Panics`** 段写清触发条件。

---
