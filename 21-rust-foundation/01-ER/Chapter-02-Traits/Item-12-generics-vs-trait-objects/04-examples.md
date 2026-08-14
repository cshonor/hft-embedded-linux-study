# Item 12 · 案例与代码

← [Item 12 目录](./README.md)

### 泛型多重 bound

```rust
fn show<T>(draw: &T)
where
    T: std::fmt::Debug + Draw,
{
    println!("{:?} has bounds {:?}", draw, draw.bounds());
}
```

只有同时实现 `Debug + Draw` 的类型才能调用。

### Trait 对象异构集合

```rust
fn render_all(items: &[&dyn Draw]) {
    for item in items {
        item.draw();
    }
}
```

### 对象安全 + `Sized` 方法

```rust
let square = Square::new(/* ... */);
let stamp: &dyn Stamp = &square;
// stamp.make_copy(); // ❌ trait object 上不可用
let copy = square.make_copy(); // ✅ 具体类型
```

---
