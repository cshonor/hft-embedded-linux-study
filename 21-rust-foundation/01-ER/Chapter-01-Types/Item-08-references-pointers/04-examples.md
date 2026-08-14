# Item 8 · 案例与代码

← [Item 8 目录](./README.md)

### 切片胖指针

```rust
let v = vec![10, 20, 30, 40];
let slice: &[i32] = &v[1..3]; // ptr 指向 v[1]，len = 2
```

`Range` → `SliceIndex` → `Index` → 构建 ptr + len。

### Trait 对象与 vtable

```rust
trait Calculate { fn add(&self, x: i32) -> i32; }

let tobj: &dyn Calculate = &concrete_impl;
// 胖指针：数据地址 + vtable（含 add 等函数指针）
```

### Deref 链（示意）

```rust
let b = Box::new(42);
let r: &i32 = &*b; // Deref: Box<i32> -> i32
```

---
