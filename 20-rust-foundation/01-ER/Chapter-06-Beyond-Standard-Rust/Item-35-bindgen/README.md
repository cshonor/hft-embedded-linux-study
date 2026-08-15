# Item 35: Prefer bindgen to manual FFI mappings

> **Effective Rust** · [Chapter 6 — Beyond Standard Rust](../../ER-本书目录.md)  
> **中文**：优先使用 bindgen 而非手动编写 FFI 映射  
> 原文：[effective-rust.com](https://www.effective-rust.com/print.html)

## 状态

- [x] 已读（笔记整理）
- [x] [item-35-bindgen](./demo-bindgen/) · [item-35-sys-workspace](./demo-sys-workspace/)

---

## 与 The Book 对照

| 主题 | 本仓库 |
|------|--------|
| `unsafe`、FFI 原则 | [19.1 不安全 Rust](../../../00-Book/19-advanced-features/19.1-不安全Rust.md) |
| 发布与构建 | [14.1 发布配置](../../../00-Book/14-cargo-crates/14.1-采用发布配置自定义构建.md) |
| FFI 边界 | [Item 34](../Item-34-ffi-boundaries/README.md) |
| 避免手写 unsafe | [Item 16](../../Chapter-03-Concepts/Item-16-avoid-unsafe/README.md) |
| CI 校验生成物 | [Item 32](../../Chapter-05-Tooling/Item-32-ci/README.md) |
| `cxx` / 工具生态 | [Item 31](../../Chapter-05-Tooling/Item-31-tooling-ecosystem/README.md) |

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
C 源码 ↔ C 头文件（C 编译器检查）
         ↓
bindgen(同一 .h) → Rust 声明
         ↓
人工手写 Rust 声明 → 易 drift，无检查
         ↓
CI：重新 bindgen → git diff 与签入版不一致则 fail（Item 32）
         ↓
xyzzy-sys（unsafe 生成码）+ xyzzy（safe 封装）→ Item 16
```

---

## 后续拓展

> 展开版：[ER-拓展索引 § Item 35](../../ER-拓展索引.md#item-35)

详见索引中各条目的完成度 `[x]` / `[ ]` 与 Book demo 链接。

---

## 速记

| 要点 | 一句 |
|------|------|
| 原则 | **bindgen > 手写** |
| 同步 | 与 C 共用 **同一 .h** |
| 架构 | **`foo-sys`** + **`foo`** safe |
| C++ | **`cxx`** |
| CI | 重生 bindgen → **diff fail** |
| 大库 | **allowlist** 只取需要的 API |

---

> **Effective Rust 全书 35 Items 笔记整理完成。**

