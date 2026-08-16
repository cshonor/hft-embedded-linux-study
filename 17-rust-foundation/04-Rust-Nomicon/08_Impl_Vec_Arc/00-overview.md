# 08 · Example: Implementing Vec · 本章定位

← [本章目录](./README.md) · [全书笔记](../notes.md) · 上一章：[07 Concurrency](../07_Concurrency_Atomic/README.md) · 下一章：[09 FFI](../09_FFI/README.md)

---

官方标题 **Example: Implementing Vec**。全书使用 **stable Rust** 从零实现标准库 `Vec` 的实战章，展示如何用 unsafe 原语构建安全、高性能底层结构。

> 同目录 **Arc** 见 [01-arc-overview.md](./01-arc-overview.md)；**Mutex** 为同章延伸，待建。

| 对照 | 路径 |
|------|------|
| 未初始化 / ptr::write | [05_Uninit_Mem](../05_Uninit_Mem/README.md) |
| OBRM / Drain 泄漏放大 | [06_OBRM_RAII](../06_OBRM_RAII/README.md) |
| ZST | [02_Data_Layout](../02_Data_Layout/00-overview.md) |
| Send/Sync | [07_Concurrency_Atomic](../07_Concurrency_Atomic/00-overview.md) |

**读完应能回答**：MyVec 三字段布局、为何用 `ptr::write/read`、`RawVec` 为何存在、ZST 为何要特判。

---

## 小节路线图

```text
01  布局 NonNull + Send/Sync
  ↓
02  RawVec 分配 · OOM · cap 上限
  ↓
03  push ptr::write · pop ptr::read
  ↓
04  Drop → dealloc
  ↓
05  Deref → slice
  ↓
06  insert/remove ptr::copy
  ↓
07  RawVec / IntoIter / Drain
  ↓
08  ZST 特判
  ↓
Arc  简化 MyArc（无 Weak）
  ↓
09 FFI
```

| 节 | 主题 | 阅读 |
|:--:|------|------|
| — | 本章定位 | 本页 |
| 1 | 数据布局 | [01-layout.md](./01-layout.md) |
| 2 | 内存分配 | [02-allocating.md](./02-allocating.md) |
| 3 | Push 与 Pop | [03-push-pop.md](./03-push-pop.md) |
| 4 | 内存释放 | [04-dealloc.md](./04-dealloc.md) |
| 5 | 切片解引用 | [05-deref.md](./05-deref.md) |
| 6 | Insert 与 Remove | [06-insert-remove.md](./06-insert-remove.md) |
| 7 | IntoIter / Drain / RawVec | [07-iterators.md](./07-iterators.md) |
| 8 | 零大小类型 | [08-zst.md](./08-zst.md) |
| — | Arc 实现要点 | [01-arc-overview.md](./01-arc-overview.md) |
| — | 速记 · 自测 |

---

## 一句话

**Vec + Arc 实战** — stable 徒手 `MyVec` 与简化 `MyArc`：NonNull/PhantomData、原子 refcount、Release/Acquire fence。

→ 从 [01-layout.md](./01-layout.md) 起读；Arc 见 [01-arc-overview.md](./01-arc-overview.md)。
