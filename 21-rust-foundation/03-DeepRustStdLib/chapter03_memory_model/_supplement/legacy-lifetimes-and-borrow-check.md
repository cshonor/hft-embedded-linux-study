# 3.2 生命周期与借用检查

> 章索引：[第 3 章](./README.md) · 前：[3.1 布局](./3.1-layout-and-alignment.md) · 后：[3.3 UnsafeCell](./3.3-unsafecell.md)

---

## 一句话

**借用检查器**在编译期保证引用不悬垂；标准库 API 里 **`'a`、`'static`、方法上的生命周期省略** 都是这套规则在 `libstd` 上的投影 — 与 [第 2 章 §2.3](../chapter02_rust_features_summary/2.3-lifetimes-in-stdlib.md) 呼应，本章从**内存模型**角度再收紧一层。

---

## 与内存模型的关系

| 机制 | 防什么 |
|------|--------|
| **`&'a T`** | `'a` 内目标内存有效 |
| **`&mut T` 独占** | 数据竞争（单线程别名写） |
| **`'static`** | 线程闭包、全局不绑栈 |
| **drop order** | 先 drop 子字段，再父结构 — 影响借用的终点 |

---

## 标准库典型签名

```rust
// 迭代器：'a 绑在被遍历容器上
pub struct Iter<'a, T: 'a> { ... }

// 线程：F: 'static — 不借短命栈
pub fn spawn<F, T>(f: F) -> JoinHandle<T>
where F: FnOnce() -> T + Send + 'static, ...
```

**为何后面要有 UnsafeCell / Mutex**：当编译期证明不够（内部可变、跨线程）时，用 **类型 + 运行时或锁** 补足 invariant。

→ [RFR Ch01 · 08 lifetimes](../../02-RFR/Chapter-01-Foundations/08-lifetimes.md) · [Nomicon 03](../../04-Rust-Nomicon/03_Lifetime_Variance/README.md)

---

## 相关

- [3.3 UnsafeCell](./3.3-unsafecell.md) — 借用规则的「受控例外」之根
