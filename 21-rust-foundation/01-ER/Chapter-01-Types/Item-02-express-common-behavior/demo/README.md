# Item 2 demo · 回调与泛型

四条重点结论的可运行示例，对应 [03-key-takeaways.md](../03-key-takeaways.md)。

```bash
cd 01-ER
cargo run -p item-02-callbacks-generics
cargo test -p item-02-callbacks-generics
```

| 示例 | 说明 |
|------|------|
| 1 | 裸 `fn` vs `Fn` 闭包捕获 |
| 2 | `FnOnce` / `FnMut` / `Fn` 选型 |
| 3 | `R: Read` 而非绑死 `File` |
| 4 | 裸 `T` vs `T: Display` |
