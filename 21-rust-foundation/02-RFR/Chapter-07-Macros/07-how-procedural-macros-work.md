# 2.4 How Do Procedural Macros Work?（过程宏如何工作）

> 所属：**Procedural Macros** · [← 章索引](./README.md)

前置 → [00-1 Token 与宏展开](./00-1-token-and-macro-pipeline.md)（声明宏 vs 过程宏 · 编译时序）

## 基本模型

```rust
// 概念示意（derive / attribute / function-like 入口不同）
#[proc_macro_derive(MyTrait)]
pub fn my_derive(input: TokenStream) -> TokenStream {
    // syn 解析 input → quote 生成 output
}
```

1. 编译器把用户写的 token 流交给 proc-macro **crate**（独立编译单元）
2. 宏函数返回新 **TokenStream**，再参与正常类型检查

## syn / quote

| crate | 作用 |
|-------|------|
| **`syn`** | 把 `TokenStream` 解析成 AST 结构 |
| **`quote`** | 把模板 / AST 转回 `TokenStream` |

## Span 与错误体验

- 每个 token 带 **`Span`**（源位置关联）
- 用 **`compile_error!`**、`proc_macro::Diagnostic`（若可用）把错误**钉在用户写的位置**，而不是宏内部匿名代码处

**Span 选择**（语义随版本演进，以官方文档为准）：

- `Span::call_site()` / `mixed_site()` / `def_site()` — 控制生成标识符解析到何处

## 与声明宏对比

| | 声明宏 | 过程宏 |
|---|--------|--------|
| 表达能力 | 模式 + 模板 | **任意 Rust** 逻辑 |
| 编译成本 | 通常较低 | 通常较高 |
| 卫生 | 局部绑定较自动 | 需手动管 **Span** |

→ demo [19.5-macros-demo](../../00-Book/19-advanced-features/19.5-macros-demo/)
