# 附录 B：运算符与符号

> **The Book** · [Appendix B（英文）](https://doc.rust-lang.org/book/appendix-02-operators.html)（Table B-1～B-10 完整表）

Rust 语法符号速查；可重载的运算符对应 `std::ops` trait。

---

## 1. 常用运算符

| 运算符 | 含义 | 可重载 trait（若适用） |
|--------|------|------------------------|
| `+` `-` `*` `/` `%` | 算术 | `Add` `Sub` `Mul` `Div` `Rem` |
| `+=` … | 复合赋值 | `AddAssign` … |
| `==` `!=` | 相等 | `PartialEq` |
| `<` `>` `<=` `>=` | 比较 | `PartialOrd` |
| `!` | 逻辑/按位非 | `Not` |
| `&&` `||` | 短路与/或 | — |
| `&` `\|` `^` | 按位与/或/异或 | `BitAnd` `BitOr` `BitXor` |
| `<<` `>>` | 移位 | `Shl` `Shr` |
| `=` | 赋值 | — |
| `?` | 错误传播 | — |
| `*`（前缀） | 解引用 | `Deref` |
| `&` / `&mut` | 借用 | — |
| `..` / `..=` | 范围（半开 / 闭） | `PartialOrd` |
| `->` | 函数返回类型 | — |
| `.` | 字段 / 方法 | — |
| `@` | 模式绑定 | — |
| `\|`（模式） | 模式多选 | — |
| `=>` | `match` 分支 | — |

---

## 2. 非运算符符号（高频）

| 符号 | 含义 |
|------|------|
| `'a` | 生命周期 / 循环标签 |
| `_` | 忽略绑定；数字分隔 |
| `\|x\| …` | 闭包 |
| `::` | 路径（`crate::` `super::` `Type::assoc`） |
| `<>` / `::<>`` | 泛型参数；**turbofish** `"42".parse::<i32>()` |
| `T: Trait` | trait 约束 |
| `T + U` | 多 trait 约束 |
| `'a + Trait` | 生命周期 + trait 约束 |
| `#![…]` / `#[…]` | 内/外属性 |
| `ident!(…)` | 宏调用 |
| `//` `///` `//!` | 注释 / 文档注释 |
| `()` `[]` `{}` | 单元、数组、块 / 结构体字面量 |

---

## 3. 与正文 / ER 对照

| 主题 | 链接 |
|------|------|
| 所有权与引用 | [第 4 章](../04-ownership/) |
| 错误 `?` | [9.2 Result](../09-error-handling/9.2-Result-与可恢复的错误.md) |
| 范围与迭代 | [13 章迭代器](../13-iterators-closures/) |
| `Deref` 强制转换 | [ER Item 05](../../01-ER/Chapter-01-Types/Item-05-type-conversions/README.md) |
| 运算符 trait | [ER Item 10](../../01-ER/Chapter-02-Traits/Item-10-standard-traits/README.md) |

---

## 记忆卡片

| 要点 | 一句 |
|------|------|
| turbofish | `::<Type>` 显式泛型实参 |
| `?` | `Result`/`Option` 早退 |
| 完整表 | 官方 Appendix B Table B-1～B-10 |
