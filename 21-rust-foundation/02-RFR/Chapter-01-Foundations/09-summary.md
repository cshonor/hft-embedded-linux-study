# 4. Summary（小结）

> [← 章索引](./README.md)

第 1 章 **Foundations** 用一套可操作的模型，回答「编译器在证明什么、拒绝什么」：

```text
Talking About Memory（术语 → 双模型 → 区域）
        ↓
Ownership（唯一负责 + Drop 顺序）
        ↓
Borrowing and Lifetimes（& / &mut / 内部可变 / 生命周期与方差）
```

## 四句话带走

1. **值 / 变量 / 指针**分清楚；赋值写的是 **place** 里的内容。  
2. **数据流**想借用，**槽位**想 unsafe 与布局。  
3. **`&T` 与 `&mut T`** 既是安全规则，也是优化契约。  
4. **生命周期 + 方差** 防止引用与类型替换越界。

## 下一章

→ [第 2 章 Types](../Chapter-02-Types/2-类型-Types-深度解析.md)：对齐、Layout、DST、trait 分发。

## 本章笔记索引

| # | 文件 |
|---|------|
| 01 | [内存术语](./01-memory-terminology.md) |
| 02 | [变量深入](./02-variables-in-depth.md) |
| 03 | [内存区域](./03-memory-regions.md) · [03.1 Rust 模型](./03-1-rust-memory-model.md) · [03.2 OS/LLVM](./03-2-os-memory-layout.md) |
| 04 | [所有权](./04-ownership.md)（[04.1](./04-1-three-rules.md)～[04.6](./04-6-pitfalls.md)） |
| 05 | [共享引用](./05-shared-references.md) |
| 06 | [可变引用](./06-mutable-references.md) |
| 07 | [内部可变性](./07-interior-mutability.md) · [07.1](./07-1-external-vs-interior.md) · [07.3 Cell/RefCell](./07-3-cell-vs-refcell.md) |
| 08 | [生命周期](./08-lifetimes.md) |
