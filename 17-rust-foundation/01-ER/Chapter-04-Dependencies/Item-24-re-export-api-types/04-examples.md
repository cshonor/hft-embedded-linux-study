# Item 24 · 案例与代码

← [Item 24 目录](./README.md)

### 跨版本调用失败

```toml
# app
rand = "=0.8.5"
dep-lib = "0.1"  # 内部 rand = "=0.7.3"
```

```rust
// dep-lib
pub fn pick_number_with<R: rand::Rng>(rng: &mut R, n: usize) -> usize { /* ... */ }

// app
let mut rng = rand::thread_rng(); // 0.8
let max: usize = rng.gen_range(5..10);
let choice = dep_lib::pick_number_with(&mut rng, max); // ❌ 类型不匹配
```

### 重导出破局

```rust
// dep-lib/lib.rs
pub use rand;

pub fn pick_number_with<R: rand::Rng>(rng: &mut R, n: usize) -> usize { /* ... */ }
```

```rust
// app
let mut prev_rng = dep_lib::rand::thread_rng(); // 0.7，与库一致
let choice = dep_lib::pick_number_with(&mut prev_rng, max); // ✅
```

---
