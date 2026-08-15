# 00.1 Token 与宏展开完整链路

> 第 7 章开篇精读 · [← 00 hub](./00-macros-overview.md) · [章索引](./README.md)

← [00 宏总览](./00-macros-overview.md) · 下一节 [00-2 宏分类](./00-2-macro-taxonomy.md)

配套 → [02 声明宏如何工作](./02-how-declarative-macros-work.md) · [07 过程宏如何工作](./07-how-procedural-macros-work.md) · Book [19.5 宏](../../00-Book/19-advanced-features/19.5-宏.md)

---

## 一、核心基础：Token 是什么

1. 编译器第一步 **词法分析（lexing）**，把整份 Rust 源代码字符串切成最小独立单元：**Token**（标识符、关键字、括号、数字、逗号、`!` 等）。
2. 整份代码变成一条 **Token 流（`TokenStream`）**；后续宏处理、语法解析、类型检查都基于 Token / AST 操作，**不会**来回变回原始字符串再 lex。
3. 宏调用也会被拆成 Token。例如 `vec![1, 2, 3]`：

```text
vec   !   [   1   ,   2   ,   3   ]
```

---

## 二、声明宏（`macro_rules!`）完整流程

### 1. 参与匹配的是哪部分 Token？

| 部分 | 是否参与模式匹配 |
|------|------------------|
| 宏名 + `!`（如 `vec!`） | ❌ **调用标记**，用于识别宏 |
| 括号内内容（如 `[1, 2, 3]`） | ✅ **参数 Token 片段**，拿去和规则匹配 |

### 2. 展开全程只有 Token，没有「先转源码文本再分词」

```text
1. 源码 → 全局分词 → 完整 Token 流
2. 识别 xxx! 宏调用 → 截取括号内参数 Token
3. 与 macro_rules! 多条规则从上到下匹配
4. 匹配成功 → 模板里 $var 替换成捕获到的原始 Token 片段
5. 输出全新 Token 流，替换掉整个宏调用的 Token
6. 若仍有嵌套宏 → 继续展开（仍是对 Token 操作）
7. 拼接回全局 Token 流 → 语法分析建 AST → 类型检查 → 单态化 → LLVM …
```

> **重点纠正**  
> 不是匹配完先生成人类可读的 Rust 源码字符串、再重新分词。  
> 编译器内部自始至终操作 **Token**；`cargo expand` 展示的展开代码是**调试等价视图**，不是运行时流程。  
> 声明宏本质：**Token 输入 → Token 模板替换 → Token 输出**。

### 3. 声明宏特点

| 项 | 说明 |
|----|------|
| 能力 | 结构化 **Token 模式匹配** + 转录 |
| 局限 | 无完整 AST 级逻辑；复杂语法校验、按字段生成做不到 |
| 开销 | 轻；`vec!` `println!` 均为声明宏 |

→ 示例：[02 §vec 模具](./02-how-declarative-macros-work.md) · [decl_macro_demo](../../00-Book/19-advanced-features/19.5-macros-demo/decl_macro_demo/)

---

## 三、过程宏：与声明宏的本质区别

### 对比总表

| | **声明宏** `macro_rules!` | **过程宏** `proc_macro` |
|---|---------------------------|-------------------------|
| **工作层级** | **词法层** — Token 模式 + 模板 | **更高层** — 编译期运行 Rust 程序处理 Token |
| **逻辑** | 静态匹配替换，**无**用户 Rust 代码执行 | **动态** Rust 代码（条件、循环、读字段） |
| **边界 API** | 规则内模板 | `TokenStream → TokenStream` |
| **典型栈** | — | `syn` 解析 AST → `quote` 生成 Token |

### 过程宏完整流程（更准确）

```text
1. 源码 → Token 流（与声明宏相同起点）
2. 编译器识别过程宏调用，把对应 TokenStream 交给独立 proc-macro crate
3. 宏作者用 Rust 处理输入（常用 syn 把 TokenStream 解析成 AST）
4. 宏函数返回新的 TokenStream（常用 quote 生成）
5. 编译器把返回的 Token 继续解析成 AST，进入类型检查等后续阶段
```

> **纠正常见偏差点**  
> 过程宏**不是**「把宏转成 Rust 源码字符串再分词」。  
> 也不是编译器直接把 **rustc 内部 AST** 交给宏 — 边界上是 **`TokenStream`**；是否解析成 AST 由宏作者决定（几乎总是 `syn`）。  
> 本质：**编译期运行一段 Rust 程序来生成代码（Token）**，表达力高于纯模板。

→ [04 过程宏类型](./04-types-of-procedural-macros.md) · [07 过程宏如何工作](./07-how-procedural-macros-work.md)

### 一句话概括

- **声明宏**：传入 Token 片段做模板匹配，换一批 Token，**没有**额外 Rust 程序执行。  
- **过程宏**：`TokenStream` 交给编译期 Rust 程序，程序跑完**输出**新 `TokenStream`。

---

## 四、宏展开在编译时序中的位置

完整顺序（不可逆）：

```text
源代码文本
  → 词法分析（全部转 Token）
  → 宏展开（声明 / 过程宏，替换 Token）
  → 语法分析（Token → AST）
  → 语法校验
  → 类型检查 / 借用检查
  → 泛型单态化
  → LLVM 后端、链接
```

1. 宏展开发生在 **类型检查、单态化之前**。  
2. 展开完毕后，才有一份固定的 Token/AST 进入类型系统。  
3. 宏只负责**改写代码结构**，不参与类型判断。

→ [00 §3 宏 vs 函数](./00-macros-overview.md#3-宏-vs-函数必背)

---

## 五、高频误区汇总

| ❌ 误区 | ✅ 正解 |
|--------|--------|
| 宏展开 = 生成 Rust 源码字符串再重新分词 | 内部全程 Token/AST 流转；`cargo expand` 只是预览 |
| 整个 `名字!(...)` 都拿去匹配规则 | 只有 **`!` 后括号内**参数 Token 参与匹配 |
| 过程宏 = 声明宏升级版，只是生成更长代码 | **模型不同**：模板替换 vs 编译期 Rust 程序 |
| 类型检查在宏展开前执行 | **必须先展开**，代码形态固定后才类型检查 |
| 过程宏入口是 rustc AST | 边界是 **`TokenStream`**；AST 由 `syn` 等库解析 |

---

## 六、核心速记

1. **Token** = 词法分析后的最小单元；宏全程在 Token 流上操作。  
2. **声明宏** = 括号内 Token 匹配 + 模板替换；`名字!` 不参与匹配。  
3. **过程宏** = 编译期 Rust 程序，`TokenStream → TokenStream`。  
4. **时序**：lex → **宏展开** → parse AST → 类型检查 → 单态化。  
5. **`cargo expand`** 看的是等价源码，不是编译器内部步骤。

---

## 速记

## Token

词法分析产物 · 整文件 = Token 流 · **不**来回变字符串

## 声明宏匹配谁？

`vec!` = 调用标记 · **`[1,2,3]`** = 参与匹配的参数 Token

## 声明宏本质

Token 进 → 模板替换 → Token 出（无「转回 .rs 再 lex」）

## 过程宏本质

`TokenStream` → 编译期 Rust 程序（常 syn/quote）→ `TokenStream`

## 编译顺序

lex → **宏展开** → parse AST → 类型检查 → 单态化 → LLVM

## 四误区

无字符串中转 · 不只括号外匹配 · 过程宏非升级版 · 类型检查在展开后

## 自测

- [ ] `vec![1,2,3]` 哪几个 Token 参与 `macro_rules!` 匹配？  
- [ ] `cargo expand` 展示的是编译器哪一步？  
- [ ] 过程宏边界 API 是 AST 还是 `TokenStream`？

