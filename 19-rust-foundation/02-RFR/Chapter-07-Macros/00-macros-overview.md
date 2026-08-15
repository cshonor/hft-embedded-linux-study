# 宏核心总览（复习用）

> 第 7 章开篇 · [← 章索引](./README.md)  
> 与 [Rust Reference · Macros](https://doc.rust-lang.org/reference/macros.html) / Book [19.5 宏](../../00-Book/19-advanced-features/19.5-宏.md) 一致。

## 开篇精读（3 专题）

| # | 专题 | 正文 | 速记 |
|:-:|------|------|------|
| **00.1** | Token 与宏展开 | [00-1 Token 链路](./00-1-token-and-macro-pipeline.md) |
| **00.2** | 宏整体分类（4 类） | [00-2 分类总览](./00-2-macro-taxonomy.md) |
| **00.3** | 宏 vs 函数 | [00-3 完整对比](./00-3-macro-vs-function.md) |

---

## 0. 怎么理解宏？（直觉模型）

### 代码生成模板

可以把宏理解成 **「代码生成模板」**：

- 你在源码里写 **`println!(...)`**、**`vec![1, 2, 3]`** 这类**宏调用**
- 编译器在**编译期**按规则**展开**成一大段普通 Rust 代码
- **运行时没有「调用宏」** — 只剩展开后的函数 / 语句 / 类型定义

比手写重复代码省事；比函数更灵活 — **`println!` 能接任意个数、任意可格式化类型**，普通函数**做不到**（函数签名固定）。

### 模式匹配 + 代码替换

你的直觉 **「输入 → 按规则匹配 → 输出对应代码」** 完全正确，更准确说是：

> **模式匹配 + 代码替换（转录）**

```text
定义宏时：  写好若干条  「若调用长这样 → 展开成那样」
调用宏时：  括号内 token 片段 → 匹配规则 → 替换成模板里的 token（非回写源码再 lex）
```

> 只有 `!` **后面括号内**的 Token 参与匹配；`名字!` 是调用标记。全程 Token 流操作 → [00-1 Token 链路](./00-1-token-and-macro-pipeline.md)

声明宏 `macro_rules!` 就是结构化 `match` → [02 声明宏如何工作](./02-how-declarative-macros-work.md)  
过程宏则是 Rust 函数处理 **`TokenStream`**，逻辑更自由 → [07 过程宏如何工作](./07-how-procedural-macros-work.md)

```text
  你写的：     vec![1, 2, 3];
                    │
                    ▼  编译最开始：词法 / 宏展开阶段
  展开成：     { let mut v = Vec::new(); v.push(1); …; v }
                    │
                    ▼  之后才做类型检查、借用检查、单态化…
  最终机器码：  与普通手写代码无异
```

### 不只处理「值」，还处理「语法结构」

函数只能：**接收已类型化的值 → 返回值**。  
宏还能在展开结果里**生成**：

- `struct` / `enum` 定义  
- `impl` 块  
- 一整段语句、`match` 分支  
- **可变参数形状**的语法（token 个数不固定）

所以 **`#[derive(Debug)]`** 不是运行期给 struct「贴标签」，而是编译期**生成**一份 `impl Debug for YourType { … }`。

### 和 Java 注解 / Lombok（get/set）像不像？

**目标相似，机制不同** — 都是**少写样板、自动生成重复代码**：

| | **Rust 宏** | **Java 注解 + APT**（如 Lombok `@Getter`） |
|---|-------------|-------------------------------------------|
| **干什么** | 编译期展开 / 生成代码 | 编译期注解处理器生成 `.java` |
| **发生阶段** | 编译**很早期**（宏展开，仍在 token / AST 层操作） | 编译期，但多在**注解处理轮次** |
| **操作对象** | **Token 流 / 语法树** — 可改结构、可变形态 | 多为类 / 字段元数据 → 生成方法 |
| **灵活度** | 声明宏 = 模式模板；过程宏 = 任意 Rust 逻辑 | 受注解 API 与处理器能力限制 |
| **运行时** | **无**宏调用开销 | **无**注解开销（生成的是普通方法） |

相似点：都是 **「写一次规则，编译器帮你吐重复代码」**。  
不同点：Rust 宏展开**更早**、能直接动 **token / 项（item）**，不局限于「给类加 getter」这类模式；调试上 Java 生成源有时更直观，Rust 需 **`cargo expand`** 看展开结果。

---

## 1. 定义

**宏 = 编译期元编程**：宏调用在**编译阶段展开**成普通 Rust 代码；**运行时没有「调用宏」这一层**（展开后的代码照常编译、运行）。

一句话：**宏是「写代码的代码」**。

---

## 2. 两大分类

| | **声明宏** | **过程宏** |
|---|------------|------------|
| **定义** | `macro_rules!` | `#[proc_macro_derive]` / `#[proc_macro_attribute]` / `#[proc_macro]` |
| **原理** | **Token 模式匹配** + 模板转录（像结构化 `match`） | Rust 函数处理 **`TokenStream`**（常配合 `syn` / `quote`） |
| **特点** | 入门快、表达力有限 | 强、灵活、可扩展语法 / DSL |
| **例子** | `vec!`、`println!`、`format!` | `#[derive(Serialize)]`、`tokio::main`、类函数宏 |

→ 分节：[01–03 声明宏](./01-when-to-use-declarative-macros.md) · [04–07 过程宏](./04-types-of-procedural-macros.md)

### 声明宏 vs 过程宏：谁更「灵活」？（别搞反）

**容易记反**：不是「声明宏更灵活」，而是 **过程宏更灵活**。

| | **声明宏** `macro_rules!` | **过程宏** `proc_macro` |
|---|---------------------------|-------------------------|
| **比喻** | **固定模具** — 规则提前写死 | **小程序** — 用 Rust 写展开逻辑 |
| **输入** | 必须 **严格匹配** 预设 token 模式 | **`TokenStream` 任意解析**（常 `syn` 成 AST） |
| **逻辑** | **静态**模式匹配 + 模板替换 | **动态**：条件、`if`、`for` 循环生成代码 |
| **能做什么** | 几种固定语法形状（如逗号分隔列表） | 读 struct 字段名/类型，每个 type **不同** `impl` |
| **搞不定时** | 不匹配 → **立刻报错**，不会「猜」 | 可写复杂校验、自定义错误信息 |

**一句话**：声明宏 = **静态 pattern match**；过程宏 = **动态代码逻辑**处理输入。

#### 例子 1：声明宏 = 固定模具（`vec!` / `my_vec!`）

模具里写死了：**「括号里是逗号分隔的表达式列表」**。

```rust
// 标准库 vec! / 本仓库 demo：my_vec!
vec![1, 2, 3];           // ✅ 匹配 $( $x:expr ),*
vec!["a", "b"];          // ✅ 元素类型可以变，形状不变
vec![];                  // ✅ 空列表也有对应分支

vec! { let x = 1; };     // ❌ 不匹配任何分支 → 编译错误
vec!(1, 2, 3);           // ❌ 圆括号不是 vec! 的模具形状
```

展开（概念上）：

```rust
// vec![1, 2, 3]  →
{
    let mut temp_vec = Vec::new();
    temp_vec.push(1);
    temp_vec.push(2);
    temp_vec.push(3);
    temp_vec
}
```

规则在 **`macro_rules!` 定义里写死** — 不会根据「你是不是 struct」换逻辑。  
→ 跟写 demo：[19.5-macros-demo · my_vec!](../../00-Book/19-advanced-features/19.5-macros-demo/decl_macro_demo/src/lib.rs)

#### 例子 2：过程宏 = 小程序（`#[derive(Debug)]`）

输入是 **整个 struct 的定义**；过程宏 **读字段名、类型**，为 **每个 struct 生成不同的** `fmt` 代码：

```rust
#[derive(Debug)]
struct Point { x: i32, y: i32 }

#[derive(Debug)]
struct Person { name: String, age: u8 }
// 两次 derive 展开的 impl Debug 内容完全不同 — 靠 Rust 代码逻辑生成
```

声明宏 **做不到**「先解析 struct 有哪些字段再生成 impl」— 它没有 AST 级别的自由逻辑，只有 **几条固定 pattern**。

#### 和「impl / derive」的对应

| 口语 | 对应 |
|------|------|
| **声明宏** | 像 **`macro_rules!` 里写死的 match 分支** — 几种输入形状，几种输出模板 |
| **过程宏** | 像 **`derive` 背后那套生成 `impl` 的 Rust 程序** — 输入一变，生成逻辑跟着变 |

→ 看展开：`cargo expand` · [第 13 章工具](../Chapter-13-Rust-Ecosystem/01-tools.md)

### 过程宏三子类

| 子类 | 形态 | 作用 |
|------|------|------|
| **派生 Derive** | `#[derive(MyTrait)]` | 为 type **追加** `impl` 等 |
| **属性 Attribute** | `#[my_attr(...)]` 修饰项 | 变换函数 / struct / mod 等 |
| **类函数 Function-like** | `my_macro!(...)` | 自定义 token 语法、DSL |

---

## 3. 宏 vs 函数（必背）

| 对比项 | **宏** | **普通函数** |
|--------|--------|--------------|
| **执行时机** | **编译期展开** | **运行期调用** |
| **参数** | 任意 **Token 流**；可变形状 | **固定签名**（类型 + 个数） |
| **能力** | 可生成 `struct` / `impl` / 语句块 | 只执行业务逻辑，**不能**生成新项 |
| **运行开销** | 无「宏调用」开销（已是内联后的代码） | 有调用（通常可内联） |
| **调试** | 报错常指向展开后；需 `cargo expand` | 栈追踪清晰 |
| **可见性** | 须在作用域内**可见**（定义或 `use` 导入） | 同模块可后定义（部分场景） |
| **类型检查** | 展开**之后**才检查 | 定义处即检查 |
| **典型用途** | 样板、可变参数语法、derive、DSL | 逻辑复用、类型安全、日常业务 |

**取舍**：**能用函数（或泛型 + trait）就不用宏** → [00-3 宏 vs 函数](./00-3-macro-vs-function.md) · [06 你真的需要宏吗](./06-so-you-think-you-want-a-macro.md)

---

## 4. FFI + HFT 怎么记

### FFI

| 场景 | 常用宏 |
|------|--------|
| 批量绑定 / 重复 `extern` 形状 | **声明宏** 或 **build.rs + bindgen** |
| `repr(C)` struct、`From`/`Into`、检查布局 | **derive 过程宏**（`bindgen` 生成、`#[derive(...)]` 封装） |
| 固定布局 | 依赖 **`repr(C)`** + [第 2 章 layout](../Chapter-02-Types/02-layout.md)，不靠宏「猜」布局 |

→ [第 11 章 FFI](../Chapter-11-Foreign-Function-Interfaces/README.md)

### HFT

| 要点 | 说明 |
|------|------|
| **零运行时宏开销** | 热路径上展开结果 = 普通代码，适合高频 |
| **别滥用** | 宏难读、难调试；**核心逻辑用函数**，宏只做生成 / 模板 |
| **编译时间** | 过程宏 + 大 derive 拖 **build**；见 [05 代价](./05-cost-of-procedural-macros.md) |

### 一句话取舍

> **函数优先；宏只用于函数做不到的事**（可变 token 形状、生成项、derive）。

---

## 5. 一页极简背诵版

```text
宏 = 编译期「模式匹配 + 代码替换」；运行时无宏调用。

直觉：代码生成模板 — println! / vec! 展开成普通 Rust。

两类：
  macro_rules!     → Token 匹配（vec!、println!）
  proc macro       → TokenStream 程序（derive / 属性 / foo!）

过程宏三种：derive · attribute · function-like

宏 vs 函数：编译期 vs 运行期；能生成 struct/impl vs 只能执行业务逻辑。

vs Java 注解：都生成样板；Rust 更早展开、更底层 token，调试看 cargo expand。

FFI：bindgen/derive + repr(C)；HFT：热路径可用展开代码，逻辑仍写函数。
```

---

## 阅读顺序

```text
00（本篇）→ 01 → … → 08 → proc-macro-workshop（可选动手）
```
