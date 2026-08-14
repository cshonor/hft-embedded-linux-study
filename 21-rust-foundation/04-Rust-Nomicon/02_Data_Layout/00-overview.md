# 02 · Data Representation in Rust · 本章定位

← [本章目录](./README.md) · [全书笔记](../notes.md) · 上一章：[01 Safe Unsafe](../01_Safe_Unsafe/README.md) · 下一章：[03 Lifetime](../03_Lifetime_Variance/README.md)

---

官方标题 **Data Representation in Rust**。底层编程中，数据在内存中的布局至关重要，并深度影响语言其它特性。本章介绍 Rust 如何处理对齐、尺寸，以及如何用 `repr` 控制布局。

| 对照 | 路径 |
|------|------|
| RFR 布局详解 | [02-layout.md](../../02-RFR/Chapter-02-Types/02-layout.md) |
| RFR 实测 demo | [layout-demo](../../02-RFR/Chapter-02-Types/layout-demo/) |
| OS 内存分区 | [03-2-os-memory-layout](../../02-RFR/Chapter-01-Foundations/03-2-os-memory-layout.md) |

**读完应能回答**：`repr(Rust)` 与 `repr(C)` 有何不同、DST/ZST/空类型是什么、何时用哪种 `repr`。

---

## 小节路线图

```text
01  repr(Rust)：对齐 · 重排 · niche
  ↓
02  DST / ZST / 空类型
  ↓
03  repr(C) / transparent / packed / align
  ↓
03 Lifetime
```

| 节 | 主题 | 阅读 |
|:--:|------|------|
| — | 本章定位 | 本页 |
| 1 | 默认布局 `repr(Rust)` | [01-repr-rust.md](./01-repr-rust.md) |
| 2 | 非常规尺寸类型 | [02-exotic-types.md](./02-exotic-types.md) |
| 3 | 替代数据表示 | [03-alt-repr.md](./03-alt-repr.md) |
| — | 速记 · 自测 |

---

## 一句话

**布局章** — 默认 `repr(Rust)` 可重排；FFI/固定格式用 `repr(C)`；DST/ZST 在 unsafe 中有特殊规则。

→ 从 [01-repr-rust.md](./01-repr-rust.md) 起读；源码见各节链到 `src/`。
