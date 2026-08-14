# Item 33 · 核心知识点

← [Item 33 目录](./README.md)

### 三层库

| 层 | 说明 |
|----|------|
| **`std`** | 默认；OS、I/O、线程、完整集合 |
| **`core`** | 始终可用；**无堆分配**；`Option`、`Result`、`Iterator`… |
| **`alloc`** | 需 `extern crate alloc;`；`Vec`、`Box`、`String`、`Rc`/`Arc` |

### `no_std`

```rust
#![no_std]
```

- 不链接 **`std`**；用于引导程序、固件、裸机。
- 许多 `std::` 基础类型实为 **`core::` 重导出**。

---
