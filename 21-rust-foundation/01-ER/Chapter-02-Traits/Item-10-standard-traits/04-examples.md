# Item 10 · 案例与代码

← [Item 10 目录](./README.md)

### `Copy` vs Move

```rust
#[derive(Clone, Copy, Debug)]
struct KeyId(u64);

let k = KeyId(42);
let k2 = k;              // 按位拷贝
println!("{:?}", k);     // ✅ 仍可用

// 无 Copy 时：let k2 = k; 后 k 不能再 use
```

### 浮点 `PartialOrd` 陷阱

```rust
#[derive(PartialOrd, PartialEq)]
struct Oddity(f32);

let x = Oddity(f32::NAN);
// x <= x 可能为 false —— 别假设自比较恒 true
```

### `Hash` + `Eq` 契约

```rust
// 若 x == y，必须 hash(x) == hash(y)
// 手写其一通常要手写或 derive 另一，且逻辑一致
```

---
