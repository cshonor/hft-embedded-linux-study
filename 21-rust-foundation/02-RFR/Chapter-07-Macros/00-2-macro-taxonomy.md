# 00.2 Rust 宏整体分类（声明宏 + 三大过程宏）

> 第 7 章开篇精读 · [← 00 hub](./00-macros-overview.md) · [章索引](./README.md)

← [00-1 Token 链路](./00-1-token-and-macro-pipeline.md) · 下一节 [00-3 宏 vs 函数](./00-3-macro-vs-function.md)

配套 → [04 过程宏类型](./04-types-of-procedural-macros.md) · [00-3 宏 vs 函数](./00-3-macro-vs-function.md) · Book [19.5 宏](../../00-Book/19-advanced-features/19.5-宏.md)

---

Rust 宏共**两大体系**：

```text
Rust 宏
├─ 声明宏 macro_rules!     →  调用：xxx!()
└─ 过程宏（3 子类）
   ├─ 派生 Derive           →  #[derive(Trait)]
   ├─ 属性 Attribute        →  #[my_attr(...)]
   └─ 类函数 Function-like  →  xxx!()   ← 与声明宏「撞脸」
```

> **易混点**：`xxx!()` 写法**既有声明宏，也有类函数过程宏** — **不能只看感叹号**区分，要看定义处是 `macro_rules!` 还是独立 `proc-macro` crate。

---

## 一、声明宏（`macro_rules!`）

俗称「**模式匹配宏**」 — Rust 原生 Token 模板宏。

```rust
macro_rules! 宏名 {
    ( 匹配模式 ) => { 展开模板 };
}
```

调用带 `!`：`my_vec![]`、`println!(...)`。

### 核心特点

| 项 | 说明 |
|----|------|
| **底层** | **Token 模式匹配 + 转录**（非纯文本拼接，非 C 预处理器） |
| **能力** | 几种固定 token 形状；**无法**遍历 AST、做复杂语义分析 |
| **项目结构** | 写在当前 crate，`#[macro_export]` 或模块内，**无需** proc-macro 子 crate |
| **开发** | 规则语法，不写完整 Rust 展开程序 |

### 示例：`my_vec!`

```rust
macro_rules! my_vec {
    () => { Vec::new() };
    ( $($x:expr),+ ) => {{
        let mut temp = Vec::new();
        $( temp.push($x); )+
        temp
    }};
}

fn main() {
    let v1 = my_vec![];
    let v2 = my_vec![10, 20, 30];
    println!("{:?}", v2);
}
```

`my_vec![]` 带 `!` → **声明宏**，不是过程宏。

→ 可运行：[19.5 · decl_macro_demo](../../00-Book/19-advanced-features/19.5-macros-demo/decl_macro_demo/) · [02 如何工作](./02-how-declarative-macros-work.md)

---

## 二、三大过程宏（须独立 `proc-macro` crate）

**本质**：编译期运行一段 Rust 函数，`TokenStream → TokenStream`（常 `syn` + `quote`）。  
能力远强于声明宏 — 条件、循环、按字段生成、编译期校验等。

### 1. 派生 Derive — `#[derive(XXX)]`

| 项 | 说明 |
|----|------|
| **形态** | 挂在 struct / enum / union 上方 |
| **作用** | 自动**追加** `impl Trait` 等 |
| **入口** | `#[proc_macro_derive(...)]` |

```rust
#[derive(Debug, Clone)]
struct User {
    id: u64,
    name: String,
}

fn main() {
    let u = User { id: 1, name: "test".into() };
    let u2 = u.clone();      // derive 生成的 Clone
    println!("{:?}", u);     // derive 生成的 Debug
}
```

自定义：`#[derive(Serialize)]` — serde 编译期生成序列化 impl。

### 2. 属性 Attribute — `#[自定义属性(参数)]`

| 项 | 说明 |
|----|------|
| **形态** | `#[xxx(args)]` 修饰函数 / struct / mod 等 |
| **作用** | **改写、包装**被修饰的整个项 |
| **入口** | `#[proc_macro_attribute]` |

```rust
#[tokio::main]
async fn main() {
    println!("异步程序启动");
}
// 展开：注入 tokio 运行时 + 同步 main 包装
```

其他：`#[get("/user")]`（路由）等。**`#[test]`** 为编译器内置属性，机制类似，非第三方 proc-macro。

### 3. 类函数 Function-like — `xxx!(参数)`

| 项 | 说明 |
|----|------|
| **形态** | `foo!(...)` — **外观与声明宏完全一致** |
| **作用** | 任意 Token 解析、DSL、编译期校验、强代码生成 |
| **入口** | `#[proc_macro]` |

```rust
// sqlx：编译期校验 SQL（需 DATABASE_URL 或 offline 缓存）
let row = sqlx::query!("SELECT id, name FROM users WHERE id = $1", 1i64)
    .fetch_one(&pool)
    .await?;
```

不是简单 Token 模板 — 要解析 SQL、做 schema/类型校验，**声明宏做不到**。

→ 详 [04 过程宏类型](./04-types-of-procedural-macros.md) · [07 如何工作](./07-how-procedural-macros-work.md)

---

## 三、重点：声明宏 vs 类函数过程宏（同为 `xxx!()`）

| 对比项 | **声明宏** `macro_rules!` | **类函数过程宏** `#[proc_macro]` |
|--------|---------------------------|----------------------------------|
| **开发** | 匹配规则 + 模板，无完整 Rust 逻辑 | 独立 proc-macro crate，Rust 解析 Token |
| **原理** | Token 模式匹配 → 模板转录 | 编程处理 Token（可 syn 成 AST） |
| **能力上限** | 固定几种语法形状 | 语法校验、语义分析、编译期外部数据（如 SQL schema） |
| **项目结构** | 当前 crate 内，零额外配置 | 必须 `proc-macro = true` 子 crate |
| **调试** | 匹配失败较直观 | 复杂；常需 `cargo expand` |
| **适用** | 轻量样板（简易 vec、重复 push） | DSL、ORM、SQL 校验、框架代码生成 |
| **类型检查** | 展开后整体检查 | 宏内可**提前**做部分校验（如 SQL 语法） |

### 直观区分

| 宏 | 类型 | 原因 |
|----|------|------|
| `vec!` / `my_vec!` | **声明宏** | Token 列表 → push 模板 |
| `sqlx::query!` / `serde_json::json!` | **类函数过程宏** | 解析 + 校验 + 复杂生成 |

---

## 四、快速判断四步法

```text
1. 定义处见 macro_rules!           →  声明宏
2. 调用 xxx!()，定义在 proc-macro crate →  类函数过程宏
3. #[derive(...)]                   →  派生过程宏
4. #[xxx(...)] 修饰项               →  属性过程宏（或内置属性）
```

**不能**仅凭 `!` 判断类型。

---

## 五、选型建议

| 需求 | 选用 |
|------|------|
| 简单模板、重复片段 | **声明宏** — 快、调试简单 |
| 语法校验、DSL、编译期查库/schema | **类函数过程宏** |
| 批量 `impl` trait | **派生过程宏** |
| 改造整个 fn/struct（async main、路由） | **属性过程宏** |
| 泛型 + trait 能表达 | **不用宏** → [00-3 宏 vs 函数](./00-3-macro-vs-function.md) |

```text
泛型+函数 → macro_rules! → 过程宏（derive / attr / foo!）
```

---

## 六、四类对照总表

| 类型 | 调用/形态 | 定义方式 | 典型 |
|------|-----------|----------|------|
| **声明宏** | `foo!(...)` | `macro_rules!` | `vec!` `println!` `my_vec!` |
| **Derive** | `#[derive(T)]` | `proc_macro_derive` | `Debug` `Serialize` |
| **Attribute** | `#[attr(...)]` 项上 | `proc_macro_attribute` | `tokio::main` `get("/")` |
| **Function-like** | `foo!(...)` | `proc_macro` | `query!` `json!` |

---

## 速记

## 两大体系

**声明宏** `macro_rules!` · **过程宏** 3 类（derive / attr / `foo!`）

## 撞脸警告

`xxx!()` = 声明宏 **或** 类函数过程宏 · **看定义，不看 !**

## 四类形态

| 类型 | 形态 |
|------|------|
| 声明 | `foo!(...)` + `macro_rules!` |
| Derive | `#[derive(T)]` |
| Attribute | `#[attr(...)]` 项上 |
| Function-like proc | `foo!(...)` + proc-macro crate |

## 声明 vs 类函数 proc

模板 Token 匹配 · 简单 · 本 crate  
vs  
Rust 程序 · DSL/SQL · 独立 proc-macro

## 判断四步

`macro_rules!` · proc crate + `!` · `derive` · `#[attr]`

## 选型口诀

简单模板 → 声明 · impl 批量 → derive · 改整项 → attr · DSL → proc `!` · 能泛型 → 不用宏

## 自测

- [ ] `vec!` 和 `json!` 分别是哪类宏？  
- [ ] 为何不能靠 `!` 区分声明宏与过程宏？  
- [ ] `#[tokio::main]` 属于哪一类？

