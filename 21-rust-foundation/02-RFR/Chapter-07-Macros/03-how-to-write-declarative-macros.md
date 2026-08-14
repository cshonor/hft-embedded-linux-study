# 1.3 How to Write Declarative Macros（如何编写声明宏）

> 所属：**Declarative Macros** · [← 章索引](./README.md)

← [02 如何工作](./02-how-declarative-macros-work.md) · 下一节 [04 过程宏类型](./04-types-of-procedural-macros.md)

前置 → [00-2 宏分类](./00-2-macro-taxonomy.md) · [02 Token 匹配与转录](./02-how-declarative-macros-work.md)

---

## 一、核心基础

1. **专属关键字**：**`macro_rules!`** — **只有声明宏**用这个定义；过程宏没有。  
2. **匹配语法不是正则** — Rust 声明宏独有的片段规则，匹配 Rust 代码片段（Token 层面）。  
3. **`$` 捕获**：`$变量名:片段分类符` — 左侧匹配捕获，右侧模板复用。

```rust
macro_rules! 宏名称 {
    (匹配模式) => { 替换生成的代码 };
    (另一种匹配模式) => { 替换生成的代码 };
}
// 调用：宏名称!(入参)
```

本质：编译期 **模式匹配 → 变量捕获 → Token 转录**（非运行期逻辑，非纯文本拼接）。

---

## 二、片段分类符（Fragment Specifiers）

`:` 后面是**片段分类符**，约束 `$` 捕获的内容必须是哪一类 Rust 语法。

| 分类符 | 含义 | 匹配内容 | 示例 |
|--------|------|----------|------|
| **`expr`** | expression | 可求值表达式 | `1+2`、`user.name`、`func()` |
| **`ident`** | identifier | 变量名、函数名、类型名（非关键字） | `age`、`user_info` |
| **`ty`** | type | 合法类型 | `i32`、`String`、`Vec<u8>` |
| **`stmt`** | statement | 完整语句（常带 `;`） | `let a = 10;`、`return;` |
| **`tt`** | token tree | 任意 `()` `[]` `{}` 包裹的一组 token — **最通用** | 递归宏、复杂嵌套 |
| **`literal`** | 字面量 | 字符串、数字、布尔字面量 | `"hello"`、`99`、`true` |
| **`pat`** | pattern | 模式（`match` 分支左侧等） | `Some(x)`、`1..=10` |
| **`block`** | block | `{ ... }` 代码块 | `{ let x = 1; x }` |
| **`item`** | item | 函数、struct、mod 等顶层项 | `fn foo() {}` |

> **`expr` 最常用，不是唯一** — 要名字用 `ident`，要类型用 `ty`。

### 1. `ident` — 动态创建变量

```rust
macro_rules! make_var {
    ($name:ident, $val:expr) => {
        let $name = $val;
    };
}

fn main() {
    make_var!(score, 95);
    println!("{}", score); // 展开 ≈ let score = 95;
}
```

### 2. `ty` — 指定类型的零值

```rust
macro_rules! default_zero {
    ($t:ty) => {
        0 as $t
    };
}

fn main() {
    let num: u64 = default_zero!(u64);
}
```

### 3. `expr` — 打印表达式

```rust
macro_rules! print_expr {
    ($e:expr) => {
        println!("结果：{}", $e);
    };
}
// print_expr!(10 * 2 + 3);
```

---

## 三、重复匹配：`$() 分隔符 量词`

匹配**多个重复参数** — `vec!`、求和宏的核心语法：

```text
$($捕获:分类符),+   或   $($捕获:分类符),*
     ↑    ↑  ↑
   重复单元 分隔符 量词
```

| 量词 | 含义 |
|------|------|
| **`+`** | 1 次及以上 |
| **`*`** | 0 次及以上 |
| **`?`** | 0 或 1 次 |

分隔符常见 `,`；也可 `;`、空格等（须与调用侧一致）。

### 求和宏

```rust
macro_rules! sum {
    () => { 0 };
    ($($x:expr),+) => {
        0 $(+ $x)+
    };
}

fn main() {
    println!("{}", sum!(1, 2, 3, 4)); // 0+1+2+3+4
}
```

### 批量创建变量

```rust
macro_rules! multi_var {
    ($($name:ident = $val:expr),*) => {
        $(let $name = $val;)*
    };
}

fn main() {
    multi_var!(a = 10, b = 20, c = 30);
    println!("{} {}", a, b);
}
```

### 复刻简易 `vec!`

```rust
macro_rules! my_vec {
    () => { Vec::new() };
    ($($x:expr),* $(,)?) => {{
        let mut v = Vec::new();
        $( v.push($x); )*
        v
    }};
}
```

→ demo：[19.5 · decl_macro_demo](../../00-Book/19-advanced-features/19.5-macros-demo/decl_macro_demo/)

---

## 四、完整结构速记

```rust
macro_rules! 宏名 {
    // 固定参数
    ($var:分类符) => { 代码里用 $var };
    // 重复多参数
    ($($var:分类符),*) => { $(复用 $var 的代码)* };
}
```

---

## 五、易错点

| # | 要点 |
|:-:|------|
| 1 | `expr` 只是其一；名字 → `ident`，类型 → `ty` |
| 2 | **匹配顺序**：从上到下；**具体模式在前，通用 / catch-all 在后** |
| 3 | 本质：编译期 Token 模式匹配 + 转录，**无运行时** |
| 4 | 只有 `macro_rules!` 是声明宏；`!()` 无此关键字且在 proc-macro crate → **类函数过程宏** → [00-2](./00-2-macro-taxonomy.md) |
| 5 | 调试：**`cargo expand`** 看展开结果 → [第 13 章工具](../Chapter-13-Rust-Ecosystem/01-tools.md) |

---

## 六、学习路线

```text
1. 吃透 expr / ident / ty — 各写 2 个小宏
2. 掌握 $()* / $()+ — 复刻简易 vec!
3. 了解 tt — 递归宏
4. 复杂语法 / 类型校验 → 过程宏，声明宏不要硬扛
```

---

## 七、匹配器与转录器（术语）

- **匹配器 (Matcher)**：片段分类符 + 重复量词  
- **转录器 (Transcriber)**：捕获的元变量填入模板生成 Token 流  

```rust
macro_rules! twice {
    ($e:expr) => {
        ($e) + ($e)
    };
}
```

---

## 八、卫生 (Hygiene)

| | 行为 |
|---|------|
| **局部绑定**（`let` 等） | 通常**卫生** — 宏内名字不意外捕获调用点同名变量 |
| **项级生成**（`fn` / `mod` / `type`） | 进入正常模块系统，**调用方可见** |

---

## 九、`$crate`

宏展开中引用**定义该宏的 crate**，避免跨 crate 路径漂移：

```rust
macro_rules! my_macro {
    () => {
        $crate::internal_helper()
    };
}
```

---

## 速记

## 结构

```rust
macro_rules! name { (pat) => { … }; }
```

## 高频分类符

| 符 | 匹配 |
|----|------|
| `expr` | 表达式 |
| `ident` | 标识符 |
| `ty` | 类型 |
| `tt` | 任意 token 树（递归宏） |

## 重复语法

`$($x:expr),+` — `+` 至少 1 · `*` 可 0 · `?` 0 或 1

## 易错

具体模式在前 · 只有 `macro_rules!` = 声明宏 · `cargo expand` 调试

## 路线

expr/ident/ty → `$()*` vec! → `tt` 递归 → 复杂上过程宏

## 自测

- [ ] `$name:ident` 和 `$e:expr` 分别能匹配什么？  
- [ ] `sum!()` 空分支为何需要 `()` 规则？  
- [ ] 声明宏和 `sqlx::query!` 如何区分？

