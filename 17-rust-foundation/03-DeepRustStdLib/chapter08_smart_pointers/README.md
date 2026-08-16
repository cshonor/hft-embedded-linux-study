# 第 8 章 · 智能指针

> 所属：[03 DeepRustStdLib](../README.md) · 前：[第 7 章 内部可变性](../chapter07_interior_mutability/README.md) · 后：[第 9 章 用户态标准库基础](../chapter09_userspace_std_basics/README.md) · 原书目录：[本书目录 § 第 8 章](../本书目录.md#第-8-章--智能指针)

**本章定位**：**ALLOC 层**堆管理核心 — **`Box` 独占** · **`RawVec`+`Vec` 动态连续** · **`Rc`/`Weak` 单线程共享** · **`Arc`/`Weak` 多线程计数** — 无 GC 下安全实现 **图/树/动态数组**；**8.6～8.8**（Cow/LinkedList/String）待刷书。

**原书主线（8.1～8.5 已写入）**：Global+Drop · RawVec/len · mem copy · 双计数+Weak 破环 · Arc 原子计数≠数据并发安全。

**阅读顺序**：**8.1 → 8.2 → … → 8.8**

---


<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **8.1** | Box<T> 类型分析 | [笔记](./8.1-box.md) |
| **8.2** | RawVec<T> 类型分析 | [笔记](./8.2-rawvec.md) |
| **8.3** | Vec<T> 类型分析 | [笔记](./8.3-vec-overview.md) |
| **8.3.1** | Vec<T> 基础分析 | [笔记](./8.3.1-vec-basics.md) |
| **8.3.2** | Vec<T> 的 Iterator Trait | [笔记](./8.3.2-vec-iterator.md) |
| **8.4** | Rc<T> 类型分析 | [笔记](./8.4-rc-overview.md) |
| **8.4.1** | Rc<T> 类型的构造函数及析构函数 | [笔记](./8.4.1-rc-ctor-dtor.md) |
| **8.4.2** | Weak<T> 类型分析 | [笔记](./8.4.2-rc-weak.md) |
| **8.4.3** | Rc<T> 的其他函数 | [笔记](./8.4.3-rc-other-fns.md) |
| **8.5** | Arc<T> 类型分析 | [笔记](./8.5-arc-overview.md) |
| **8.5.1** | Arc<T> 类型的构造函数及析构函数 | [笔记](./8.5.1-arc-ctor-dtor.md) |
| **8.5.2** | Weak<T> 类型分析 | [笔记](./8.5.2-arc-weak.md) |
| **8.5.3** | Arc<T> 的其他函数 | [笔记](./8.5.3-arc-other-fns.md) |
| **8.6** | Cow<'a, T> 类型分析 | [笔记](./8.6-cow-overview.md) |
| **8.6.1** | ToOwned Trait 分析 | [笔记](./8.6.1-toowned-trait.md) |
| **8.6.2** | Cow<'a, T> 分析 | [笔记](./8.6.2-cow.md) |
| **8.7** | LinkedList<T> 类型分析 | [笔记](./8.7-linkedlist.md) |
| **8.8** | String 类型分析 | [笔记](./8.8-string-overview.md) |
| **8.8.1** | 初识 String 类型分析 | [笔记](./8.8.1-string-basics.md) |
| **8.8.2** | 格式化字符串分析 | [笔记](./8.8.2-format-string.md) |

<!-- /AUTO:SECTION-INDEX -->
## 子节索引

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **8.1** | `Box<T>` 类型分析 | ✅ |
| **8.2** | `RawVec<T>` 类型分析 | ✅ |
| **8.3** | `Vec<T>` 类型分析 | ✅ |
| **8.3.1** | `Vec<T>` 基础分析 | ✅ |
| **8.3.2** | `Vec<T>` 的 Iterator Trait | ✅ |
| **8.4** | `Rc<T>` 类型分析 | ✅ |
| **8.4.1** | `Rc<T>` 类型的构造函数及析构函数 | ✅ |
| **8.4.2** | `Weak<T>` 类型分析 | ✅ |
| **8.4.3** | `Rc<T>` 的其他函数 | ✅ |
| **8.5** | `Arc<T>` 类型分析 | ✅ |
| **8.5.1** | `Arc<T>` 类型的构造函数及析构函数 | ✅ |
| **8.5.2** | `Weak<T>` 类型分析 | ✅ |
| **8.5.3** | `Arc<T>` 的其他函数 | ✅ |
| **8.6** | `Cow<'a, T>` 类型分析 | 📝 规划 |
| **8.6.1** | `ToOwned` Trait 分析 | 📝 规划 |
| **8.6.2** | `Cow<'a, T>` 分析 | 📝 规划 |
| **8.7** | `LinkedList<T>` 类型分析 | 📝 规划 |
| **8.8** | `String` 类型分析 | 📝 规划 |
| **8.8.1** | 初识 `String` 类型分析 | 📝 规划 |
| **8.8.2** | 格式化字符串分析 | 📝 规划 |

---

## 与主线对照

| 本章 | 本仓库延伸 |
|------|------------|
| `Vec` / `RawVec` | [1.2 alloc 库](../chapter01_std_overview/1.2-alloc-crate.md) · [Nomicon 08](../../04-Rust-Nomicon/08_Impl_Vec_Arc/README.md) |
| `Rc` / `Weak` / 环 | [8.4.2 · Weak 破环](./8.4.2-rc-weak.md) |
| `Arc` + 并发 | [第 11 章 并发](../chapter11_concurrency/README.md) · [05-atomic](../../05-Async-Concurrency-Network/01-atomic/README-学习区.md) |
| `String` | [第 6 章 §6.4](../chapter06_basic_types_continued/README.md) |
| Iterator | [第 5 章 迭代器](../chapter05_iterators/README.md) |

---

## HFT 阅读提示

| 节 | 实盘关联 |
|----|----------|
| **8.2～8.3** | 预分配 `Vec`、控制 `reserve`、避免热路径 realloc |
| **8.5～8.5.3** | 跨线程共享只读行情；`Arc::clone` 原子成本 |
| **8.6** | `Cow` 借视图 vs 按需拥有，减少符号表克隆 |
| **8.8.2** | 格式化日志与 `format!` 分配开销 |
