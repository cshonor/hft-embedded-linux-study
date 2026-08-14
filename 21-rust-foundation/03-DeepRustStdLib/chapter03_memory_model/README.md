# 第 3 章 · 内存操作

> 所属：[03 DeepRustStdLib](../README.md) · 前：[第 2 章 Rust 特征小议](../chapter02_rust_features_summary/README.md) · 后：[第 4 章 基本类型](../chapter04_primitive_types/README.md)

**本章定位**：所有权、借用、生命周期 **最终都可归结为内存操作**。六大板块：**裸指针** · **MaybeUninit/ManuallyDrop** · **NonNull/Unique** · **mem** · **堆分配** · **所有权/型变理论** — **HFT 内存池与无锁结构的核心理论章**。

**原书主线（已写入核心节）**：3.1 90% unsafe=ptr→& · 3.2 未初始化 · 3.4/3.5 封装指针 · 3.6 take/forget · 3.7 Layout/Global · 3.10 型变 · 3.11 回顾。

**阅读顺序**：**3.1 → 3.2 → … → 3.11**

---

<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **3.1** | 裸指针——不安全的根源 | [3.1-raw-pointers.md](./3.1-raw-pointers.md) |
| **3.1.1** | 裸指针具体实现 | [3.1.1-raw-pointer-impl.md](./3.1.1-raw-pointer-impl.md) |
| **3.1.2** | 固有模块裸指针关联函数 | [3.1.2-raw-pointer-inherent-fns.md](./3.1.2-raw-pointer-inherent-fns.md) |
| **3.1.3** | 裸指针操作 | [3.1.3-raw-pointer-ops.md](./3.1.3-raw-pointer-ops.md) |
| **3.1.4** | 裸指针番外 | [3.1.4-raw-pointer-extra.md](./3.1.4-raw-pointer-extra.md) |
| **3.2** | MaybeUninit\<T\>——未初始化变量方案 | [3.2-maybeuninit.md](./3.2-maybeuninit.md) |
| **3.2.1** | MaybeUninit\<T\> 定义 | [3.2.1-maybeuninit-definition.md](./3.2.1-maybeuninit-definition.md) |
| **3.2.2** | ManuallyDrop\<T\> 定义 | [3.2.2-manuallydrop-definition.md](./3.2.2-manuallydrop-definition.md) |
| **3.2.3** | MaybeUninit\<T\> 构造函数 | [3.2.3-maybeuninit-constructors.md](./3.2.3-maybeuninit-constructors.md) |
| **3.2.4** | MaybeUninit\<T\> 初始化函数 | [3.2.4-maybeuninit-init-fns.md](./3.2.4-maybeuninit-init-fns.md) |
| **3.2.5** | MaybeUninit\<T\> 数组类型操作 | [3.2.5-maybeuninit-array.md](./3.2.5-maybeuninit-array.md) |
| **3.2.6** | 典型案例 | [3.2.6-maybeuninit-cases.md](./3.2.6-maybeuninit-cases.md) |
| **3.3** | 裸指针再论 | [3.3-raw-pointers-revisited.md](./3.3-raw-pointers-revisited.md) |
| **3.4** | 非空裸指针——NonNull\<T\> | [3.4-nonnull.md](./3.4-nonnull.md) |
| **3.4.1** | 构造关联函数 | [3.4.1-nonnull-constructors.md](./3.4.1-nonnull-constructors.md) |
| **3.4.2** | 类型转换函数 | [3.4.2-nonnull-conversions.md](./3.4.2-nonnull-conversions.md) |
| **3.4.3** | 其他函数 | [3.4.3-nonnull-other-fns.md](./3.4.3-nonnull-other-fns.md) |
| **3.5** | 智能指针的基座——Unique\<T\> | [3.5-unique.md](./3.5-unique.md) |
| **3.6** | mem 模块函数 | [3.6-mem-module.md](./3.6-mem-module.md) |
| **3.6.1** | 构造泛型变量函数 | [3.6.1-mem-construct.md](./3.6.1-mem-construct.md) |
| **3.6.2** | 泛型变量所有权转移函数 | [3.6.2-mem-ownership-transfer.md](./3.6.2-mem-ownership-transfer.md) |
| **3.6.3** | 其他函数 | [3.6.3-mem-other-fns.md](./3.6.3-mem-other-fns.md) |
| **3.7** | 动态内存申请及释放 | [3.7-heap-alloc.md](./3.7-heap-alloc.md) |
| **3.7.1** | 内存布局 | [3.7.1-memory-layout.md](./3.7.1-memory-layout.md) |
| **3.7.2** | 动态内存申请与释放接口 | [3.7.2-alloc-free-api.md](./3.7.2-alloc-free-api.md) |
| **3.8** | 全局变量内存探讨 | [3.8-static-memory.md](./3.8-static-memory.md) |
| **3.9** | drop 总结 | [3.9-drop-summary.md](./3.9-drop-summary.md) |
| **3.10** | 所有权、生命周期、借用探讨 | [3.10-ownership-lifetimes-borrow.md](./3.10-ownership-lifetimes-borrow.md) |
| **3.11** | 回顾 | [3.11-recap.md](./3.11-recap.md) |

<!-- /AUTO:SECTION-INDEX -->

> 各节 `.md` 为**笔记本体**；旧版误编号笔记见 [_supplement/](./_supplement/README.md)。

---

## 与主线对照

| 本章 | 本仓库延伸 |
|------|------------|
| 裸指针 / unsafe | [RFR Ch09](../../02-RFR/Chapter-09-Unsafe-Code/README.md) · [Nomicon 01](../../04-Rust-Nomicon/01_Safe_Unsafe/README.md) |
| MaybeUninit | [Nomicon 05](../../04-Rust-Nomicon/05_Uninit_Mem/README.md) · [_supplement/legacy-maybeuninit](./_supplement/legacy-maybeuninit.md) |
| 堆分配 | [1.2 alloc 库](../chapter01_std_overview/1.2-alloc-crate.md) · [Nomicon 08](../../04-Rust-Nomicon/08_Impl_Vec_Arc/README.md) |

---

## HFT 阅读提示

| 节 | 实盘关联 |
|----|----------|
| **3.2** | 预分配内存池、ring buffer 槽位、避免 zero-init |
| **3.7** | 自定义分配器、对齐与缓存行 |
| **3.9** | 手动 `drop`、避免双重释放 |
