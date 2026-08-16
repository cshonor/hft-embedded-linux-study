# Item 34: Control what crosses FFI boundaries

> **Effective Rust** · [Chapter 6 — Beyond Standard Rust](../../ER-本书目录.md)  
> **中文**：控制跨越 FFI 边界的内容  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [x] [item-34-ffi-box](./demo/)

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| `unsafe`、裸指针 | [19.1 不安全 Rust](../../../00-Book/19-advanced-features/19.1-不安全Rust.md) |
| 所有权 | [4.1 什么是所有权](../../../00-Book/04-ownership/4.1-什么是所有权.md) |
| `Drop` / RAII | [Item 11](../../Chapter-02-Traits/Item-11-drop-raii/README.md) |
| 生命周期、引用 | [Item 14](../../Chapter-03-Concepts/Item-14-lifetimes/README.md) |
| 避免手写 unsafe | [Item 16](../../Chapter-03-Concepts/Item-16-avoid-unsafe/README.md) |
| Panic 与 FFI | [Item 18](../../Chapter-03-Concepts/Item-18-dont-panic/README.md) |
| FFI 多版本 ODR | [Item 25](../../Chapter-04-Dependencies/Item-25-dependency-graph/README.md) |
| 自动生成绑定 | [Item 35](../Item-35-bindgen/README.md)（待补） |

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
Rust 侧：借用 + 生命周期（Item 14/15）
         ↓ 过 FFI
C 侧：裸指针，无借期、可 UAF、可 data race
         ↓
重建安全：unsafe 在内，safe wrapper 对外（Item 16）
         ↓
所有权跨界：同侧 alloc/free；Box ↔ 裸指针（Item 11 Drop）
         ↓
panic 不得越过 FFI（Item 18 → catch_unwind / abort）
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 34](../../ER-拓展索引.md#item-34)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| ABI | **`extern "C"`** + **`#[repr(C)]`** |
| 内存 | **谁分配谁释放** |
| 整数 | 固定宽度；慎 `c_int` |
| 封装 | unsafe 在内，**safe API** 在外 |
| Box | **`into_raw` / `from_raw`** |
| 字符串 | **`CString` / `CStr`** |
| panic | **不过 FFI** |
| 绑定 | 手写危险 → **bindgen** |

