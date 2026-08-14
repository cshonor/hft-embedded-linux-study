# layout-demo

RFR 第 2 章 · [02 布局](../02-layout.md) 配套可运行示例。

对比 `Test`、`DefaultOrder`、enum niche、**DST 宽指针**、`MyStruct` 的 `size_of` / offset / 字节 dump。

```bash
# 在仓库根目录
cargo run --manifest-path 02-RFR/Chapter-02-Types/layout-demo/Cargo.toml
```

使用 **stable** Rust 即可（`offset_of!` 已稳定）。
