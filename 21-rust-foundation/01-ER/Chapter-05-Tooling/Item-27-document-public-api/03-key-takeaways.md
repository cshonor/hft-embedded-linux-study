# Item 27 · 重点结论

← [Item 27 目录](./README.md)

### 不要复述签名

- ❌ 「接收 `&BoundingBox`，返回新 `BoundingBox`」—— 签名已说清，重构易过时。
- ✅ **意图、不变量、边界、为何这样设计**。

### 写「为什么」，不写「谁在用」

- ❌ 「模块 A 目前用此方法做 X」—— A 重构后文档即垃圾。
- ✅ **面向未来的语义**与使用约束。

### Markdown 与交叉引用

- 标识符用 `` `code` ``；链接用 `` [`SomeType`] `` → rustdoc 可跳转。

### 编译器护栏

```rust
#![deny(broken_intra_doc_links)]  // 死链 → 编译错误
#![warn(missing_docs)]            // pub 项缺文档 → 警告
```

---
