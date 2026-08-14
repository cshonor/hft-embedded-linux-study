# 05 · Working With Uninitialized Memory · 本章定位

← [本章目录](./README.md) · [全书笔记](../notes.md) · 上一章：[04 Type Cast](../04_Type_Cast/README.md) · 下一章：[06 OBRM](../06_OBRM_RAII/README.md)

---

官方标题 **Working With Uninitialized Memory**。运行时分配的内存初始均为**未初始化**；在未初始化时将其解释为任何类型的值 → **UB**。本章讲解 Safe / Unsafe 两侧如何管理这些区域。

| 对照 | 路径 |
|------|------|
| RFR unsafe 调用 | [03-calling-unsafe-functions](../../02-RFR/Chapter-09-Unsafe-Code/03-calling-unsafe-functions.md) |
| no_std 动态内存 | [02-dynamic-memory-allocation](../../02-RFR/Chapter-12-Rust-Without-Standard-Library/02-dynamic-memory-allocation.md) |
| 实现 Vec（后续） | [08_Impl_Vec_Arc](../08_Impl_Vec_Arc/README.md) |

**读完应能回答**：Safe 如何静态阻止读未初始化栈变量、Drop flags 解决什么、`MaybeUninit` 与 `ptr::write` 的分工。

---

## 小节路线图

```text
01  Safe 受检：分支分析 · move-out → checked.rs
  ↓
02  Drop flags（条件 Drop）→ drop_flags.rs
  ↓
03  MaybeUninit + ptr 突破 Safe → maybe_uninit.rs / ptr_ops.rs
  ↓
04  与 Vec 实现的关系 → ch08
  ↓
06 OBRM
```

| 节 | 主题 | 阅读 |
|:--:|------|------|
| — | 本章定位 | 本页 |
| 1 | 安全受检 | [01-checked.md](./01-checked.md) |
| 2 | drop flags | [02-drop-flags.md](./02-drop-flags.md) |
| 3 | 非受检（MaybeUninit + ptr） | [03-unchecked.md](./03-unchecked.md) |
| 4 | 与后续 Vec 关系 | [04-vec-prelude.md](./04-vec-prelude.md) |
| — | 速记 · 自测 |

---

## 一句话

**未初始化内存章** — Safe 分支分析 vs 逻辑 move-out、Drop flags、MaybeUninit 标准路径、`ptr::write` / `copy` 底层操作。

→ 从 [01-checked.md](./01-checked.md) 起读；源码从各节链到 `src/`。
