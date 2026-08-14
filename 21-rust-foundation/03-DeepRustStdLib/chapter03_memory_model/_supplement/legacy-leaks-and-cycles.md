# 3.9 内存泄漏与循环引用

> 章索引：[第 3 章](./README.md) · 前：[3.8 PhantomData](./3.8-phantomdata.md) · 后：[3.10 MaybeUninit](./3.10-maybeuninit.md)

---

## 一句话

Rust **「安全」不保证不泄漏** — **`mem::forget`、`Rc` 循环引用、长生命周期 `Arc`  hoarding** 都会让内存永不释放；读 `std` 智能指针时必须会 **破环** 与 **弱引用**。

---

## 泄漏 vs UB

| | **内存泄漏** | **UB（悬垂/use-after-free）** |
|--|--------------|-------------------------------|
| **安全 Rust 能否发生** | ✅ 能（故意 leak） | ❌ 不能（编译期+类型系统防） |
| **典型原因** | `Rc` 环、`forget`、通道/线程未 join 但进程常驻 | 仅 `unsafe` 误用 |
| **HFT 影响** | 长跑策略内存涨、池子假「满」 | 崩溃、错误报价 |

---

## 循环引用

```text
Rc<A> ──► Rc<B>
  ▲           │
  └───────────┘   强计数均 ≥ 1 → 永不 drop
```

| 解法 | 说明 |
|------|------|
| **`Weak<T>`** | 打破 `Rc` 环；`upgrade()` 得 `Option<Rc>` |
| **重新设计所有权** | 父 owns 子，子不 own 父 |
| **`Arc` + Mutex** | 仍可能环 — 须显式拆环 |

与 [3.5 RefCell](./3.5-refcell.md)、[3.6 Mutex](./3.6-mutex.md) 组合：`Rc<RefCell<_>>`、`Arc<Mutex<_>>` 是环的常见载体。

---

## 标准库相关 API

- **`std::mem::forget`** — 故意不 drop（泄漏语义）。
- **`Rc::strong_count` / `weak_count`** — 调试环。
- **`Drop`** — 正常路径 RAII 释放。

→ [Book 15 智能指针](../../00-Book/15-smart-pointers/) · [Nomicon 06 OBRM](../../04-Rust-Nomicon/06_OBRM_RAII/README.md)

---

## 相关

- [第 4 章 智能指针（规划）](../README.md#目录)
- [3.10 MaybeUninit](./3.10-maybeuninit.md) — 池回收 vs 真泄漏
