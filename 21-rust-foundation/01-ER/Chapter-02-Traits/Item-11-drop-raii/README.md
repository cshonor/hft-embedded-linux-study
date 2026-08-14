# Item 11: Implement the Drop trait for RAII patterns

> **Effective Rust** · [Chapter 2 — Traits](../../ER-本书目录.md)  
> **中文**：为 RAII 模式实现 Drop trait  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [x] demo：[15.3-drop-demo](../../../00-Book/15-smart-pointers/15.3-drop-demo/)（`-- custom` · `-- socket` · `-- guard` · `-- manual`）

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| RAII、`Drop` | [15.3](../../../00-Book/15-smart-pointers/15.3-使用Drop运行清理代码.md) · [15.3.1](../../../00-Book/15-smart-pointers/15.3.1-Drop顺序与进阶场景.md) · [15.3.2 Socket RAII](../../../00-Book/15-smart-pointers/15.3.2-Drop与网络Socket-RAII.md) |
| `MutexGuard` 示例 | [16.3 共享状态并发](../../../00-Book/16-fearless-concurrency/16.3-共享状态并发.md) |
| 所有权、作用域 | [4.1 所有权](../../../00-Book/04-ownership/4.1-什么是所有权.md) |
| FFI 资源 | [Item 34](../../Chapter-06-Beyond-Standard-Rust/Item-34-ffi-boundaries/README.md)（ER） |

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
C/C++ 手动 lock/unlock、malloc/free
  → 早退 / panic 易漏释放
Rust：构造拿资源 + Drop 放资源（词法作用域）
  → MutexGuard、File、Box 等 RAII 包装
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 11](../../ER-拓展索引.md#item-11)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| RAII | 构造拿、析构放；实例在才可用资源 |
| `Drop` | 编译器自动调；别手写 `x.drop()` |
| 提前结束 | `std::mem::drop(x)` |
| 失败释放 | 用 `release()`；Drop 兜底 |
| 锁 | 小作用域 `{}` 尽早解锁 |

