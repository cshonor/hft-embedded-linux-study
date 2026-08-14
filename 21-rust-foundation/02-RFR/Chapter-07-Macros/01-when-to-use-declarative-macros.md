# 1.1 When to Use Declarative Macros（何时使用声明宏）

> 所属：**Declarative Macros** · [← 章索引](./README.md)

← [00-3 宏 vs 函数](./00-3-macro-vs-function.md) · 下一节 [02 如何工作](./02-how-declarative-macros-work.md)

前置 → [00-2 宏分类](./00-2-macro-taxonomy.md) · [00-3 宏 vs 函数](./00-3-macro-vs-function.md)

> ER → [Item 28 · 审慎使用宏](../../01-ER/Chapter-05-Tooling/Item-28-macros-judiciously/README.md)

---

**定义载体**：`macro_rules!`，调用带 **`!`** — `vec!` · `println!` · `assert!`

**核心总纲**：**函数 / 泛型 / Trait 优先，声明宏是兜底方案**

---

## 一、核心用途：削样板（Boilerplate）

多段代码**骨架完全一致**，仅局部 Token / 语法片段不同 — 复制粘贴维护成本高，适合用声明宏批量生成。

### 典型场景

| # | 场景 | 说明 |
|:-:|------|------|
| 1 | **批量测试模板** | 共用初始化 / 断言 / 清理，仅输入输出不同 |
| 2 | **多类型相似 `impl`** | Getter/Setter、错误转换、newtype 包装 |
| 3 | **错误 / 日志模板** | 自动带文件名行号、变参 — `bail!` `ensure!` |
| 4 | **变长参数语法** | 函数参数固定；宏用 `$()*` 捕获任意个数 — `vec!` `format!` |
| 5 | **极简嵌入式 DSL** | 自定义调用语法，突破函数传参限制 |

### 样板示例

```rust
// ❌ 重复手写
struct A; impl Foo for A { fn run() {} }
struct B; impl Foo for B { fn run() {} }
struct C; impl Foo for C { fn run() {} }

// ✅ 声明宏批量生成
macro_rules! impl_foo {
    ($($ty:ident),*) => {
        $( impl Foo for $ty { fn run() {} } )*
    };
}
impl_foo!(A, B, C);
```

→ 片段分类符：[03 如何编写](./03-how-to-write-declarative-macros.md)

---

## 二、黄金法则：泛型 vs 声明宏

### 1. 优先【泛型 / 普通函数 / Trait】

**判定**：差异仅在**类型层面**，语法结构统一，换类型参数即可。

**优势**：编译器原生支持 · 报错精准 · IDE 跳转/重构 · 维护成本低。

```rust
fn identity<T>(x: T) -> T { x }
fn max<T: Ord>(a: T, b: T) -> T { if a > b { a } else { b } }
```

### 2. 必须【声明宏 `macro_rules!`】

**判定**：差异在**语法形状 / Token 结构**，泛型无法覆盖：

1. 任意数量**变长参数**  
2. 生成顶层 **Item**：`struct` / `impl` / `match` / `static` / 函数块  
3. 自定义**多分支** token 模式  
4. 捕获源码位置：`file!()` · `line!()` · `column!()`  
5. 输入 Token 结构不固定，无法用类型参数抽象  

```rust
vec![];
vec![1, 2, 3];
vec![0.1, 0.2, 0.3];
// 不定长逗号分隔表达式 — 语法层面差异，泛型函数做不到
```

---

## 三、决策树：你真的需要声明宏吗？

```text
1. 普通函数能否实现？
   固定参数、纯值运算 → 用函数，不用宏

2. 泛型 + Trait 能否抽象？
   仅类型不同、结构统一 → 用泛型，不用宏

3. 是否匹配声明宏场景？
   批量 impl · 变长参数 · 自定义语法 · 行号捕获 → macro_rules!

4. 声明宏仍不够？
   读 struct 字段、复杂 AST → 过程宏 → [06](./06-so-you-think-you-want-a-macro.md) · [04](./04-types-of-procedural-macros.md)
```

```text
函数 → 泛型/Trait → 声明宏 → 过程宏
```

---

## 四、泛型 vs 声明宏 对比表

| 对比维度 | 泛型 / Trait / 函数 | 声明宏 `macro_rules!` |
|----------|---------------------|------------------------|
| **抽象层级** | 类型层面 | 源码 Token / 语法层面 |
| **参数数量** | 编译期固定 | 变长（`$()*`） |
| **代码生成** | 单态化，不新增顶层项 | 可生成 `impl` / `struct` 等 |
| **报错** | 精准 | 常指向展开后；需 `cargo expand` |
| **IDE** | 跳转 / 重构完善 | 较弱 |
| **适用边界** | 逻辑同、仅类型不同 | 结构同、Token 形状不同 |
| **运行开销** | 零成本单态化 | 编译期展开，运行时无宏调用 |

---

## 五、不该用声明宏的场景

1. **纯类型复用、数值运算** — 泛型函数  
2. **重复 ≤2 处** — 复制粘贴比抽象宏更省  
3. **复杂业务分支** — 宏内难调试；逻辑拆成普通函数，宏只包样板  
4. **运行时反射 / 动态配置** — 宏仅编译期  
5. **深度解析 struct 字段** — **过程宏 derive** → [04](./04-types-of-procedural-macros.md)

---

## 六、配套延伸

| 主题 | 链接 |
|------|------|
| ER Item 28 | 审慎使用宏，无替代再引入 |
| `cargo expand` | [第 13 章工具](../Chapter-13-Rust-Ecosystem/01-tools.md) |
| 卫生性 Hygiene | [03 §八](./03-how-to-write-declarative-macros.md) |
| 分层选型 | 函数 → 泛型 → 声明宏 → 过程宏 |

---

## 七、核心总结

1. 声明宏 = 编译期 Token 模式匹配，**消除语法层面样板**。  
2. **黄金顺序**：函数 > 泛型/Trait > 声明宏 > 过程宏。  
3. **核心区分**：泛型解「类型差异」，宏解「语法/Token 结构差异」。  
4. **独有能力**：变长参数、批量 `impl`、行号捕获、迷你 DSL。  
5. **短板**：报错晦涩、IDE 弱 — **能不用就不用**。

---

## 速记

## 总纲

**函数 / 泛型 / Trait 优先 · 声明宏兜底**

## 适用（削样板）

测试模板 · 批量 impl · 日志/错误宏 · 变长参数 · 迷你 DSL

## 黄金区分

| 泛型 | 声明宏 |
|------|--------|
| 类型差异 | 语法/Token 形状差异 |

## 决策树

函数 → 泛型 → `macro_rules!` → 过程宏

## 声明宏独有能力

变长 `$()*` · 生成 impl/struct · `line!()` · 自定义 token 模式

## 不用宏

纯类型逻辑 · 重复≤2处 · 复杂业务 · 运行时反射 · 读字段→derive

## 自测

- [ ] `vec!` 为何不能用泛型函数替代？  
- [ ] `impl_foo!(A,B,C)` 解决的是哪类样板？  
- [ ] 声明宏不够时下一步选什么？

