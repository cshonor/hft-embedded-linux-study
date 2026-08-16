# 附录 A：关键字

> **The Book** · [Appendix A（英文）](https://doc.rust-lang.org/book/appendix-01-keywords.html)

关键字是 Rust **保留**的标识符，不能用作函数名、变量名、类型名等（除非用 **`r#` 原始标识符**）。

---

## 1. 当前在用关键字（按用途分组）

| 类别 | 关键字 |
|------|--------|
| 控制流 | `if` `else` `match` `loop` `while` `for` `break` `continue` `return` |
| 函数 / 类型 | `fn` `struct` `enum` `union` `trait` `impl` `type` `const` `static` |
| 模块 / 可见性 | `mod` `use` `pub` `crate` `super` `self` `Self` |
| 绑定 / 可变性 | `let` `mut` `ref` `move` |
| 异步 | `async` `await` |
| 其他 | `as` `dyn` `extern` `in` `unsafe` `where` `true` `false` |

完整释义见官方附录（每条关键字一行说明）。

---

## 2. 预留关键字（暂无功能）

`abstract` `become` `box` `do` `final` `gen` `macro` `override` `priv` `try` `typeof` `unsized` `virtual` `yield`

---

## 3. 原始标识符 `r#`

当名字与关键字冲突，或跨 **Edition** 调用旧 crate  API 时使用：

```rust
fn r#match(needle: &str, haystack: &str) -> bool {
    haystack.contains(needle)
}

fn main() {
    assert!(r#match("foo", "foobar"));
}
```

- `try` 在 2015 不是关键字，2018+ 是；调用 2015 edition 库的 `try` 需写 `r#try`。
- Edition 详解 → [E.5 Editions](./E.5-Editions.md)

---

## 4. 与正文对照

| 章节 | 关键字示例 |
|------|------------|
| [3.1 变量](../03-common-concepts/3.1-变量和可变性.md) | `let` `mut` `const` |
| [6 枚举](../06-enums-pattern-matching/) | `enum` `match` |
| [7 模块](../07-packages-modules/) | `mod` `pub` `use` `crate` |
| [10 泛型](../10-generics-traits-lifetimes/) | `where` `impl` `trait` |
| [19.1 unsafe](../19-advanced-features/19.1-不安全Rust.md) | `unsafe` |

---

## 记忆卡片

| 要点 | 一句 |
|------|------|
| 保留字 | 不能当普通标识符 |
| `r#` | 强制使用关键字作名字 |
| Edition | 同一关键字在不同 edition 可能不同（如 `try`） |
