# Item 33 · 案例与代码

← [Item 33 目录](./README.md)

### `no_std` + 可选 `alloc`

```rust
#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(feature = "alloc")]
extern crate alloc;

#[cfg(feature = "alloc")]
use alloc::vec::Vec;
```

### Fallible 构建 `Vec`

```rust
fn try_build_a_vec() -> Result<Vec<u8>, String> {
    let mut v = Vec::new();
    let required_size = 4;
    v.try_reserve(required_size)
        .map_err(|_| format!("Failed to allocate {} items!", required_size))?;
    v.push(1);
    Ok(v)
}
```

### `no_std` 下常见替代

| `std` 有 | `no_std` 替代 |
|----------|----------------|
| `HashMap` / `HashSet` | **`BTreeMap` / `BTreeSet`** |
| `Mutex` / `RwLock` | **`spin`** 等自旋锁 crate |
| `std::thread` | 平台特定 / 无 |

---
