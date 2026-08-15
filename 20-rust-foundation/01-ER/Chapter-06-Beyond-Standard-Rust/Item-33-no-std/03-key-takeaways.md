# Item 33 · 重点结论

← [Item 33 目录](./README.md)

### CI 是唯一可靠守卫

- 无意间依赖带 `std` 的 crate → **编译器不专门警告**。
- CI：`cargo build --no-default-features --target thumbv6m-none-eabi`（或项目实际 target）。

### Feature 隔离（加法！）

```toml
[features]
default = ["std"]
std = ["alloc"]
alloc = []
```

```rust
#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(feature = "alloc")]
extern crate alloc;
```

- ❌ **`no_std` feature 删代码** — Item 26：unification 并集会炸。
- ✅ **`std` feature 加能力**。

### OOM 与 fallible allocation

- `alloc` 默认：**分配失败 → panic**（`Vec::push`）。
- 嵌入式：用 **`try_reserve`**、**`Box::try_new`** 等返回 `Result`。

---
