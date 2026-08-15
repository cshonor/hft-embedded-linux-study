# Item 30 · 案例与代码

← [Item 30 目录](./README.md)

### `black_box` 防优化

```rust
b.iter(|| {
    let result = factorial(std::hint::black_box(15));
    assert_eq!(result, 1_307_674_368_000);
});
```

无 `black_box` → 编译期算好 → 假快；有 → ~真实 **ns/iter**。

### 集成测试骨架

```rust
// tests/integration_test.rs
use my_crate::public_api;

#[test]
fn smoke() {
    assert!(public_api().is_ok());
}
```

### Doc test（Item 27）

````rust
/// ```
/// assert_eq!(my_crate::add(2, 2), 4);
/// ```
````

`cargo test` 自动编译运行。

---
