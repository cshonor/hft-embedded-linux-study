# Item 12 · 重点结论

← [Item 12 目录](./README.md)

| 优先 | 场景 |
|------|------|
| **默认用泛型** | 性能、组合表达、编译期已知类型 |
| **用 `dyn Trait`** | 异构集合（`Vec<&dyn Shape>`）；严控二进制体积/编译时间；类型编译期未知（`dlopen` 等） |

### `where Self: Sized` 技巧

```rust
trait Stamp: Draw {
    fn make_copy(&self) -> Self where Self: Sized;
}
```

- `&dyn Stamp` 合法；
- **`stamp.make_copy()`** 在 trait object 上**不可调用**（仅具体类型上可调）。

---
