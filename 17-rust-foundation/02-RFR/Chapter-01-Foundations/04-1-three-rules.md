# 04.1 · 三条核心规则

← [04 所有权索引](./04-ownership.md) · 下一节：[04-2-move-copy-clone.md](./04-2-move-copy-clone.md)

---

| # | 规则 | 含义 |
|---|------|------|
| 1 | **每个值有且只有一个所有者** | 同一时刻只有一个绑定「负责 drop」；避免多指针重复释放、悬垂 |
| 2 | **所有者离开作用域时，值被 drop** | OBRM / RAII：自动调 `Drop`，释放堆/关闭句柄等 → [Book 15.3.0 RAII/OBRM](../../00-Book/15-smart-pointers/15.3.0-RAII与OBRM辨析.md) |
| 3 | **所有权可转移（move）；非 `Copy` 类型不能隐式复制** | `let s2 = s1;` 后 `s1` 失效（若 `String`）；`i32` 等 `Copy` 类型除外 |

```rust
let s1 = String::from("hello");
let s2 = s1;   // move：堆上字符串的所有权 s1 → s2
// println!("{s1}"); // ❌ s1 不再负责这块堆
```

→ Move / Copy 细节：[04-2-move-copy-clone.md](./04-2-move-copy-clone.md)
