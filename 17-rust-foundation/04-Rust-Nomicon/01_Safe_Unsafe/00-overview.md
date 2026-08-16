# 01 · Meet Safe and Unsafe · 本章定位

← [本章目录](./README.md) · [全书笔记](../notes.md) · 下一章：[02 Data Layout](../02_Data_Layout/README.md)

---

官方标题 **Meet Safe and Unsafe**。正文无「第一章」编号，但这是全书第一个核心主题板块，为后续 unsafe 编程建立心智模型。

| 对照 | 路径 |
|------|------|
| The Book unsafe 入门 | [19.1 不安全 Rust](../../00-Book/19-advanced-features/19.1-不安全Rust.md) |
| RFR 深入 | [第 9 章 Unsafe Code](../../02-RFR/Chapter-09-Unsafe-Code/README.md) |
| ER 能不用就不用 | [Item 16 avoid unsafe](../../01-ER/Chapter-03-Concepts/Item-16-avoid-unsafe/README.md) |

**读完应能回答**：Safe 与 Unsafe 的边界在哪、`unsafe` 关键字两种用法、Unsafe 额外五种能力、信任为何不对称、为何安全性是非局部的。

---

## 小节路线图

```text
01  为何分 Safe/Unsafe（C++ 对比 · HFT 场景）
  ↓
02  unsafe 契约：声明 vs 履行（Vec::push 案例）
  ↓
03  五种高危能力 → five_powers.rs
  ↓
04  信任不对称 · 非局部性 → privacy.rs
  ↓
05  易错 FAQ
  ↓
06  HFT 实操规范
  ↓
07  裸指针 *const / *mut 深度解读 → raw_pointers.rs
  ↓
速记（见 README 末）
```

| 节 | 主题 | 阅读 |
|:--:|------|------|
| — | 本章定位 | 本页 |
| 一 | 设计出发点 | [01-why-safe-unsafe.md](./01-why-safe-unsafe.md) |
| 二 | unsafe 两种作用 | [02-unsafe-contract.md](./02-unsafe-contract.md) |
| 三 | 五种高危能力 | [03-five-powers.md](./03-five-powers.md) |
| 四 | 信任与非局部性 | [04-trust-and-nonlocality.md](./04-trust-and-nonlocality.md) |
| 五 | 易错疑问 | [05-faq.md](./05-faq.md) |
| 六 | HFT 实操规范 | [06-hft-practice.md](./06-hft-practice.md) |
| 七 | 裸指针完整解读 | [07-raw-pointers.md](./07-raw-pointers.md) |
| — | 速记 · 自测 |

---

## 一句话

**Safe = 编译器兜底内存安全；Unsafe = 放开 5 种能力、程序员自行维护不变量** — 不是独立语言，所有权/借用仍是底层约束。

→ 深度总结从 [01-why-safe-unsafe.md](./01-why-safe-unsafe.md) 起读；源码从 [03-five-powers.md](./03-five-powers.md) 对照 [five_powers.rs](./src/five_powers.rs)。
