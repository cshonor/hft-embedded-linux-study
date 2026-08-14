# 2. Ownership（所有权）

> 所属：**Ownership** · [← 章索引](./README.md)

所有权保证：每个值在某一责任域内被**唯一**地负责释放（**RAII** / **`Drop`**）。

物理背景 → [03.1 Rust 内存模型 · 栈/堆/静态](./03-1-rust-memory-model.md) · 术语 → [01 内存术语](./01-memory-terminology.md)

---

## 三条核心规则（速览）

| # | 规则 | 含义 |
|---|------|------|
| 1 | **每个值有且只有一个所有者** | 同一时刻只有一个绑定「负责 drop」 |
| 2 | **所有者离开作用域时，值被 drop** | RAII：自动调 `Drop` |
| 3 | **所有权可转移（move）；非 `Copy` 类型不能隐式复制** | `let s2 = s1;` 后 `s1` 失效（`String`）；`i32` 等 `Copy` 除外 |

→ 展开：[04-1 三条规则](./04-1-three-rules.md)

---

## 子节导航

| 节 | 主题 | 阅读 |
|:--:|------|------|
| 04.1 | 三条核心规则 | [04-1-three-rules.md](./04-1-three-rules.md) |
| 04.2 | Move / Copy / Clone | [04-2-move-copy-clone.md](./04-2-move-copy-clone.md) |
| 04.3 | Drop 基础 · `Box` · 自定义 Drop | [04-3-drop.md](./04-3-drop.md) |
| 04.4 | Drop 顺序（局部 vs 字段） | [04-4-drop-order.md](./04-4-drop-order.md) |
| 04.5 | 引用与 move · panic / unwind | [04-5-refs-and-panic.md](./04-5-refs-and-panic.md) |
| 04.6 | 易错点 · 延伸 | [04-6-pitfalls.md](./04-6-pitfalls.md) |
| — | 速记 · 自测 |

---

## 阅读顺序

```text
04.1  三条规则
  ↓
04.2  Move / Copy / Clone（Copy trait，非栈/堆决定）
  ↓
04.3  Drop 是什么 · Box
  ↓
04.4  Drop 顺序（逆声明 vs 正字段 — 易混）
  ↓
04.5  引用不拥有 · panic 仍 drop
  ↓
04.6  易错点 → 05 共享引用
```

---

## 一句话

> **Move / Copy 由 `Copy` trait 决定，不由栈/堆决定** — 无 `Copy` 则 move；要 duplicate 堆 → `.clone()`。

---

## 对照阅读

- Book → [4.1 什么是所有权](../../00-Book/04-ownership/4.1-什么是所有权.md)
- 内存三分类 → [03.1 Rust 模型](./03-1-rust-memory-model.md)
- 下一节 → [05 共享引用](./05-shared-references.md)（借用不抢所有权）
