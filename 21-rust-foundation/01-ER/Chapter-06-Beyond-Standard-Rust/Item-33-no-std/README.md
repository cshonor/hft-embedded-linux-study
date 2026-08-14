# Item 33: Consider making library code no_std compatible

> **Effective Rust** · [Chapter 6 — Beyond Standard Rust](../../ER-本书目录.md)  
> **中文**：考虑让库代码兼容 no_std  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [x] [item-33-no-std](./demo/)

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| `core` 与所有权基础 | [4.1 所有权](../../../00-Book/04-ownership/4.1-什么是所有权.md) |
| `Mutex` / 并发（no_std 缺） | [16.3 共享状态](../../../00-Book/16-fearless-concurrency/16.3-共享状态并发.md) |
| `HashMap`（no_std 常缺） | [8.3 HashMap](../../../00-Book/08-collections/8.3-hashmap.md) |
| Features 加法原则 | [Item 26](../../Chapter-04-Dependencies/Item-26-feature-creep/README.md) |
| 零拷贝 vs 易用（嵌入式硬约束） | [Item 20](../../Chapter-03-Concepts/Item-20-avoid-over-optimize/README.md) |
| CI 交叉编译 | [Item 32](../../Chapter-05-Tooling/Item-32-ci/README.md) |

---

## 一句话

见 [03-key-takeaways.md](./03-key-takeaways.md)。

---

## 专项笔记（按需点开）

| # | 专题 | 阅读 |
|---|------|------|
| 01 | 核心知识点 | [01-core-concepts.md](./01-core-concepts.md) |
| 02 | 逻辑脉络 | [02-logic-flow.md](./02-logic-flow.md) |
| 03 | 重点结论 | [03-key-takeaways.md](./03-key-takeaways.md) |
| 04 | 案例与代码 | [04-examples.md](./04-examples.md) |
| 05 | 易错细节 | [05-pitfalls.md](./05-pitfalls.md) |


---

## 逻辑脉络

```text
库兼容 no_std → 嵌入式 / 内核用户也能用
         ↓
常只需 std:: → core:: 替换
         ↓
无 OS：无 HashMap（随机种子）、无 Mutex（OS 原语）
         ↓
用 BTreeMap、spin 等替代
         ↓
Feature "std" / "alloc" 加法门控（Item 26）
         ↓
CI --target thumbv6m-none-eabi 守卫（Item 32）
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 33](../../ER-拓展索引.md#item-33)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| 栈 | **core**（无堆）→ **alloc**（堆）→ **std**（OS） |
| 声明 | `#![cfg_attr(not(feature="std"), no_std)]` |
| Feature | **`std` 加法**，别 `no_std` 减法 |
| 守卫 | **CI 交叉编译** bare-metal |
| OOM | **`try_reserve`** / `try_new` |
| 缺啥 | HashMap→BTree；Mutex→spin |

