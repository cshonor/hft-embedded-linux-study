# 附录 C：可 derive 的 Trait

> **The Book** · [Appendix C（英文）](https://doc.rust-lang.org/book/appendix-03-derivable-traits.html)

`#[derive(...)]` 为 struct/enum **自动生成** trait 实现。标准库中**仅下列 trait** 可用 derive；`Display` 等需手写。

```rust
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
struct Point { x: i32, y: i32 }
```

第三方 crate 也可用过程宏扩展 derive（见 [19.5 宏](../19-advanced-features/19.5-宏.md)）。

---

## 标准库可 derive 列表

| Trait | derive 后可用 | 要点 |
|-------|---------------|------|
| **Debug** | `{:?}`、`assert_eq!` 失败打印 | 程序员调试输出 |
| **PartialEq** | `==` `!=` | 字段全等才相等 |
| **Eq** | 标记「自反相等」 | 需先有 `PartialEq`；`f64` 有 `NaN` 不能 `Eq` |
| **PartialOrd** | `<` `>` `<=` `>=` | 需 `PartialEq`；`NaN` 无序 |
| **Ord** | 全序 `cmp` | 需 `PartialOrd + Eq`；`BTreeSet` 键 |
| **Clone** | `.clone()` 深拷贝 | 各字段也需 `Clone` |
| **Copy** | 栈上按位复制 | 需 `Clone`；无自定义方法 |
| **Hash** | `.hash()` | `HashMap` 键；字段均 `Hash` |
| **Default** | `Default::default()` | 字段均 `Default`；配合 struct update `..` |

---

## 不能 derive 的典型例子

| Trait | 原因 |
|-------|------|
| **Display** | 面向用户的格式无统一默认 |
| **From / Into** | 转换语义因类型而异 → 手写或 `derive_more`（[ER Item 05](../../01-ER/Chapter-01-Types/Item-05-type-conversions/README.md)） |
| **Drop** | 资源释放逻辑需定制（[15 章智能指针](../15-smart-pointers/)） |

---

## 与正文对照

| 章节 | trait |
|------|-------|
| [5.1 结构体](../05-structs/) | `Debug` |
| [5.3 结构体方法](../05-structs/) | `Debug` |
| [10.2 Trait](../10-generics-traits-lifetimes/10.2-定义与实现Trait.md) | 默认方法 vs derive |
| [ER Item 06 newtype](../../01-ER/Chapter-01-Types/Item-06-newtype-pattern/README.md) | derive + 手写转发 |

---

## 记忆卡片

| 要点 | 一句 |
|------|------|
| derive 集合 | Debug, Eq/Ord, Clone/Copy, Hash, Default |
| 比较链 | `PartialEq` → `Eq`；`PartialOrd` → `Ord` |
| Copy | 栈复制；Clone 可含堆分配 |
