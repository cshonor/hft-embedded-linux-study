# 第 7 章 · 内部可变性类型

> 所属：[03 DeepRustStdLib](../README.md) · 前：[第 6 章 基本类型（续）](../chapter06_basic_types_continued/README.md) · 后：[第 8 章 智能指针](../chapter08_smart_pointers/README.md) · 原书目录：[本书目录 § 第 7 章](../本书目录.md#第-7-章--内部可变性类型)

**本章定位**：内部可变性 **不是绕过编译器的补丁** — **`UnsafeCell` → Cell/RefCell`**（运行期借用）· **`Pin`/`Unpin`**（自引用/async）· **`OnceCell`/`Lazy`**（单次/延迟 init）— 为树、图、状态机留 **合法动态空间**。

**原书主线（已写入）**：7.1 Borrow · 7.2～7.3 内部 mut · 7.4 Pin · 7.5 Lazy/OnceCell。

**阅读顺序**：**7.1 → 7.2 → … → 7.5**

---


<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **7.1** | Borrow / BorrowMut 分析 | [笔记](./7.1-borrow-borrowmut.md) |
| **7.2** | Cell<T> 类型分析 | [笔记](./7.2-cell-overview.md) |
| **7.2.1** | UnsafeCell<T> 分析 | [笔记](./7.2.1-unsafecell.md) |
| **7.2.2** | Cell<T> 分析 | [笔记](./7.2.2-cell.md) |
| **7.3** | RefCell<T> 类型分析 | [笔记](./7.3-refcell-overview.md) |
| **7.3.1** | Borrow Trait 分析 | [笔记](./7.3.1-refcell-borrow-trait.md) |
| **7.3.2** | BorrowMut Trait 分析 | [笔记](./7.3.2-refcell-borrowmut-trait.md) |
| **7.3.3** | RefCell<T> 的其他函数 | [笔记](./7.3.3-refcell-other-fns.md) |
| **7.4** | Pin<T> / Unpin<T> 类型分析 | [笔记](./7.4-pin-unpin.md) |
| **7.5** | Lazy<T> 类型分析 | [笔记](./7.5-lazy.md) |

<!-- /AUTO:SECTION-INDEX -->
## 子节索引

### 7.1 `Borrow` / `BorrowMut`

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **7.1** | `Borrow` / `BorrowMut` 分析 | ✅ |

### 7.2 `Cell<T>`

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **7.2** | `Cell<T>` 类型分析 | ✅ |
| **7.2.1** | `UnsafeCell<T>` 分析 | ✅ |
| **7.2.2** | `Cell<T>` 分析 | ✅ |

↔ [7.2.1 · UnsafeCell](./7.2.1-unsafecell.md) · [3.1 · unsafe 根源](../chapter03_memory_model/3.1-raw-pointers.md)

### 7.3 `RefCell<T>`

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **7.3** | `RefCell<T>` 类型分析 | ✅ |
| **7.3.1** | `Borrow` Trait 分析 | ✅ Ref guard |
| **7.3.2** | `BorrowMut` Trait 分析 | ✅ RefMut guard |
| **7.3.3** | `RefCell<T>` 的其他函数 | ✅ |

↔ [7.3 · RefCell 总览](./7.3-refcell-overview.md)

### 7.4～7.5

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **7.4** | `Pin<T>` / `Unpin<T>` 类型分析 | ✅ |
| **7.5** | `Lazy<T>` 类型分析 | ✅ OnceCell |

---

## 与主线对照

| 本章 | 本仓库延伸 |
|------|------------|
| 内部可变性总览 | [RFR 07 内部可变性](../../02-RFR/Chapter-01-Foundations/07-interior-mutability.md) |
| `Pin` | [RFR Ch10 异步](../../02-RFR/Chapter-10-Asynchronous-Programming/README.md) · [05-async Pin](../../05-Async-Concurrency-Network/) |
| `Mutex` / `RwLock` | 非本章 — 见补充轨道 `chapter03_std_sync_supplement/`（规划） |

---

## HFT 阅读提示

| 节 | 实盘关联 |
|----|----------|
| **7.2** | 单线程热路径计数、标志位（无原子开销） |
| **7.3** | 单线程订单簿可变借用；**勿**跨线程用 `RefCell` |
| **7.4** | 自引用结构、async 状态机 |
| **7.5** | 静态配置 / 符号表延迟初始化 |
