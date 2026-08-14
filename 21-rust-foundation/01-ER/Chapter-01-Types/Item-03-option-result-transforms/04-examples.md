# Item 3 · 案例与代码

← [Item 3 目录](./README.md)

### `map_err` + `?` 压缩 I/O 错误

```rust
// 手写 match：每个 Err 都要 return
// match std::fs::File::open("/etc/passwd") { ... }

let f = std::fs::File::open("/etc/passwd")
    .map_err(|e| format!("Failed: {:?}", e))?;
```

### 借用 + `unwrap_or` 的生命周期陷阱

```rust
// ❌ Vec 非 Copy，不能从 &Option<Vec<u8>> 里 move 出来
// encrypt(&self.payload.unwrap_or(vec![]));

// ✅ 在引用层解包
encrypt(self.payload.as_ref().unwrap_or(&vec![]));
```

### 演进对比（示意）

```rust
// 偏冗长
match maybe {
    Some(v) => use_value(v),
    None => default(),
}

// 更紧凑
if let Some(v) = maybe {
    use_value(v);
}
let v = maybe.map(use_value).unwrap_or_default();
```

---
