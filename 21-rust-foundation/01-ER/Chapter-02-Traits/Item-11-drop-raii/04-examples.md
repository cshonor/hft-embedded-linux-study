# Item 11 · 案例与代码

← [Item 11 目录](./README.md)

```rust
{
    let mut v = self.value.lock().unwrap(); // RAII guard
    *v += delta;
} // drop(&mut guard) → 自动 unlock
```

- 不能「忘了 unlock」；
- 不能「没锁就改数据」（类型系统 + guard 代理）。

---
