# Item 16: Avoid writing unsafe code

> **Effective Rust** · [Chapter 3 — Concepts](../../ER-本书目录.md)  
> **中文**：避免编写不安全代码  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [x] demo：[item-16-miri](./demo/) + CI `miri` job

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| `unsafe` 五类超能力、封装原则 | [19.1 不安全 Rust](../../../00-Book/19-advanced-features/19.1-不安全Rust.md) |
| 智能指针内部实现 | [15 章](../../../00-Book/15-smart-pointers/)、[Item 8](../../Chapter-01-Types/Item-08-references-pointers/README.md) |
| `Send`/`Sync` 与并发 | [16.4 Send 与 Sync](../../../00-Book/16-fearless-concurrency/16.4-Send与Sync.md) |
| FFI 边界（后续） | [Item 34](../../Chapter-06-Beyond-Standard-Rust/Item-34-ffi-boundaries/README.md)、[Item 35](../../Chapter-06-Beyond-Standard-Rust/Item-35-bindgen/README.md) |

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
Rust 卖点：零开销 + 编译期内存安全
         ↓
底层需求：OS API、硬件、极限性能 → 需要「逃生舱」unsafe
         ↓
Item 16 立场：避免 **编写 (writing)**，优先 **复用 (reuse)**
         ↓
std / crates.io 里已有专家封装 → 用安全 API，别重写 unsafe
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 16](../../ER-拓展索引.md#item-16)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| 立场 | **复用 > 重写** unsafe |
| 责任 | 编译器不管 → 你证明不变量 |
| 封装 | unsafe 在内，safe API 在外 |
| 工具 | Safety comment + Miri + 更多测试 |
| FFI | 一律当不可信；见 Item 34/35 |

