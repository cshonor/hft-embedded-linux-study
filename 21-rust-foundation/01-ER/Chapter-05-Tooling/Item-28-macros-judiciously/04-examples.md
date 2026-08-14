# Item 28 · 案例与代码

← [Item 28 目录](./README.md)

### 微型 DSL：HTTP 状态码

- 一次宏调用集中：**数值 + 分组 + 文案**。
- 展开 → enum、`TryFrom`、分组函数、文本 lookup — **单一事实来源**。

### 注入诊断位置

```rust
// 展开处自动带 file!() / line!()
panic!("assert failed at {}:{}", file!(), line!());
```

宏在**调用点**展开 → 位置信息准确，无需手传。

### 重复求值陷阱（声明宏）

```rust
macro_rules! square {
    ($e:expr) => { { $e * $e } };
}
// square!( { x += 1; x } )  → x += 1 执行两次！
```

修复：

```rust
($e:expr) => { { let __val = $e; __val * __val } };
// 或限制为 $i:ident
```

---
