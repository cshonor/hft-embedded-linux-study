# Item 22 · 案例与代码

← [Item 22 目录](./README.md)

### struct 字段与方法须逐一公开

```rust
pub mod somemodule {
    #[derive(Debug, Default)]
    pub struct AStruct {
        count: i32,        // 私有
        pub name: String,  // 显式 pub
    }

    impl AStruct {
        fn canonical_name(&self) -> String { /* ... */ } // 私有
        pub fn id(&self) -> String { /* ... */ }          // 公开
    }
}

// 外部：s.count、canonical_name() → private field / private method
```

### `pub(in path)` — std 迭代器适配器

- 内部模块 `std::iter::adapters` 用 **`pub(in crate::iter)`** 限制方法可见性。
- 外层 **`pub use`** 再给用户扁平 API（见 Item 24）。

---
