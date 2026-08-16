# 2.1 Types of Procedural Macros（过程宏类型）

> 所属：**Procedural Macros** · [← 章索引](./README.md)

← [03 如何编写声明宏](./03-how-to-write-declarative-macros.md) · 下一节 [05 代价](./05-cost-of-procedural-macros.md)

前置 → [00-1 Token 与宏展开](./00-1-token-and-macro-pipeline.md) · [07 过程宏如何工作](./07-how-procedural-macros-work.md)

---

过程宏 = **编译期运行的 Rust 函数**：入参 **`TokenStream`**，出参 **`TokenStream`**（常配合 **`syn`** / **`quote`**）。  
三者底层共性：独立 **`proc-macro = true`** crate · 编译阶段执行 · 输出 Token 参与后续 parse / 类型检查。

→ 四大类总览：[00-2 宏分类总览](./00-2-macro-taxonomy.md) · 声明 vs 类函数 `!` 撞脸对照

```text
用户源码 Token  →  proc-macro crate（Rust 程序）  →  新 Token 流  →  正常编译
```

> **别和声明宏混**：`vec!` `println!` 是 **`macro_rules!` 声明宏**，不是过程宏。  
> 只有带 `#[proc_macro_derive]` / `#[proc_macro_attribute]` / `#[proc_macro]` 的才是过程宏。

---

## 一、派生 Derive

| 项 | 说明 |
|----|------|
| **语法** | `#[derive(MyTrait)]` 修饰 struct / enum / union |
| **作用** | 自动为类型**追加** `impl` 等代码（一般不改动用户手写项主体） |
| **典型** | `Debug` `Clone` `Serialize` `Deserialize` `PartialEq` |

```rust
#[derive(Debug, Clone)]
struct Point { x: i32, y: i32 }
// 展开 ≈ impl Debug for Point { … } + impl Clone for Point { … }
```

- 入口：`#[proc_macro_derive(MyTrait, attributes(...))]`  
- 输入：被 derive 的**整个类型定义** Token  
- 输出：追加的 `impl` / 辅助项 Token  

→ demo：[19.5 · hello_macro_derive](../../00-Book/19-advanced-features/19.5-macros-demo/hello_macro_derive/)

---

## 二、属性 Attribute

| 项 | 说明 |
|----|------|
| **语法** | `#[my_attr(args)]` 写在函数 / struct / mod / 字段等**上方** |
| **作用** | **修饰**目标项 — 改写、包装、注入额外代码 |
| **典型** | `#[tokio::main]` · 路由 `#[get("/path")]` · 测试封装 |

```rust
#[tokio::main]
async fn main() { /* 展开为同步 main + 运行时启动 */ }

#[my_attr(log)]
fn handler() { /* 属性宏可改写函数签名/体 */ }
```

- 入口：`#[proc_macro_attribute]`  
- 输入：**属性参数 Token** + **被修饰项 Token**（两个 `TokenStream`）  
- 输出：替换后的项 Token  

> `#[test]` 等内置属性由编译器提供，机制类似「属性宏」，但不必来自 proc-macro crate。

---

## 三、类函数 Function-like

| 项 | 说明 |
|----|------|
| **语法** | `my_macro!(tokens)` — 外观像函数调用 + `!` |
| **作用** | 接收**任意 Token 流**，自定义语法、构建小型 DSL |
| **典型（过程宏）** | `serde_json::json!` · `sqlx::query!` · 自定义 `include_html!` |

```rust
let v = serde_json::json!({ "name": "Rust", "year": 2015 });
// 过程宏在编译期解析 JSON 字面量 → 生成构造 Value 的代码
```

- 入口：`#[proc_macro]`  
- 输入：括号内 **Token 流**  
- 输出：替换宏调用的 **Token 流**  

### 与声明宏 `macro_rules!` 的区分（易混）

| | **声明宏** `macro_rules!` | **类函数过程宏** `#[proc_macro]` |
|---|---------------------------|-----------------------------------|
| 例子 | `vec!` `format!` `println!` | `json!` `query!` |
| 逻辑 | Token 模式 + 模板 | Rust 程序 + `syn`/`quote` |
| 能力 | 固定几种 token 形状 | 任意解析与生成 |

**形状相同**（都是 `foo!(...)`），**底层完全不同** — 看 crate 是 `macro_rules!` 还是 `proc-macro`。

---

## 四、三类对照总表

| 子类 | 形态 | 输入 | 典型场景 |
|------|------|------|----------|
| **Derive** | `#[derive(T)]` | 类型定义 | 序列化、Debug、Clone、ORM 实体 |
| **Attribute** | `#[attr(...)]` 项上 | 属性 + 被修饰项 | 运行时入口、路由、AOP 包装 |
| **Function-like** | `foo!(...)` | 括号内 Token | DSL、SQL、JSON 字面量、编译期求值 |

---

## 五、Span 与卫生（预览）

过程宏的「卫生」**不自动**等同声明宏；生成标识符须管 **`Span`** → [07 如何工作](./07-how-procedural-macros-work.md)

---

## 速记

## 三类（全是 proc-macro）

| 子类 | 语法 | 作用 |
|------|------|------|
| **Derive** | `#[derive(T)]` | 追加 `impl` |
| **Attribute** | `#[attr(...)]` 项上 | 改写/包装目标项 |
| **Function-like** | `foo!(...)` | 自定义 Token / DSL |

## 共性

`TokenStream → TokenStream` · 独立 proc-macro crate · 编译期运行 Rust

## 易混

`vec!` `format!` = **声明宏** · `json!` `query!` = **过程宏**（同为 `!` 语法）

## 入口属性

`proc_macro_derive` · `proc_macro_attribute` · `proc_macro`

## 自测

- [ ] `#[derive(Serialize)]` 属于哪类？输入是什么 Token？  
- [ ] `vec!` 是过程宏还是声明宏？  
- [ ] 属性宏比 derive 多接收哪一部分输入？

