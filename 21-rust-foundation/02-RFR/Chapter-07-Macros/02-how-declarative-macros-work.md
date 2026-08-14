# 1.2 How Declarative Macros Work（声明宏如何工作）

> 所属：**Declarative Macros** · [← 章索引](./README.md)

直觉：受约束的「**模式匹配 + 转录**」，发生在 **TokenTree** 层面 — **不是** C 预处理器的纯文本拼接。

前置 → [00-1 Token 与宏展开](./00-1-token-and-macro-pipeline.md)（词法 · 匹配谁 · 无字符串中转）

## 展开管线

1. 调用 `foo!(...)` → 编译器收集 **token 流**
2. 与 **`macro_rules!`** 的**匹配器**比对
3. 匹配成功 → **转录器**生成新 token 流
4. 展开结果必须是**合法 Rust 片段**（表达式 / 项 / 类型等）

## TokenTree

- 输入被切成 **TokenTree**，**不必**已是完整合法 Rust 程序，但须是合法 token 流
- 因此比「任意文本替换」安全，但仍可能生成**语义错误**的合法语法

## 与过程宏对比（预览）

| | 声明宏 | 过程宏 |
|---|--------|--------|
| 定义 | `macro_rules!` | `#[proc_macro]` 等 Rust 函数 |
| 逻辑 | **静态**模式 + 模板 | **动态** Rust 代码处理 `TokenStream` |
| 输入 | **必须匹配**预设 pattern | 可解析任意合法项（struct、属性参数…） |
| 灵活度 | 低 — 几种固定形状 | 高 — 条件、循环、按字段生成 |

→ [00 §2 固定模具 vs 小程序](./00-macros-overview.md#声明宏-vs-过程宏谁更灵活别搞反) · [07 过程宏如何工作](./07-how-procedural-macros-work.md)

## `vec!` 式声明宏：输入形状写死在 pattern 里

`macro_rules!` 的每条分支 = **「输入必须长这样」**：

```rust
macro_rules! my_vec {
    () => { Vec::new() };
    ( $( $x:expr ),* $(,)? ) => {{
        let mut temp_vec = Vec::new();
        $( temp_vec.push($x); )*
        temp_vec
    }};
}
```

| 调用 | 结果 |
|------|------|
| `my_vec![1, 2, 3]` | ✅ 匹配 `$( $x:expr ),*` |
| `my_vec!["a", "b"]` | ✅ 元素类型可变，**逗号列表形状**不变 |
| `my_vec![]` | ✅ 匹配 `()` 分支 |
| `my_vec! { stmt; }` | ❌ 无匹配分支 → **macro expansion error** |

**不是**「声明宏输入种类无限」— 而是 **在模具允许的几种形状内**，元素个数/类型可以变（靠 `$(...)*` 重复）。  
超出模具 = 直接报错，不会像过程宏那样写 parser 去「理解」新语法。

→ 可运行：[19.5-macros-demo · decl_macro_demo](../../00-Book/19-advanced-features/19.5-macros-demo/decl_macro_demo/) · 编写详解：[03 片段分类符](./03-how-to-write-declarative-macros.md)
