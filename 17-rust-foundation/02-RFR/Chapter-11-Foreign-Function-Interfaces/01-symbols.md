# 1.1 Symbols（符号）

> 所属：**Crossing Boundaries with extern** · [← 章索引](./README.md)

## 声明外部符号

```rust
extern "C" {
    fn c_function(x: i32) -> i32;
    static C_GLOBAL: i32;
}
```

在其他翻译单元（常为 C）中定义；Rust 侧 **`unsafe` 调用**。

## 导出 Rust 符号

- **`#[no_mangle]`** — 禁用 name mangling，链接器按约定名解析。
- **`#[export_name = "..."]`** — 与 C 侧命名不一致时的映射。

## 链接

- **`#[link(name = "crypto")]`** — 指定库名
- **`#[link_name = "..."]`** — 符号级重命名

Book → [19.1 不安全 Rust · extern](../../00-Book/19-advanced-features/19.1-不安全Rust.md)
