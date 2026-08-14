# 第 6 章 · 基本类型（续）

> 所属：[03 DeepRustStdLib](../README.md) · 前：[第 5 章 迭代器](../chapter05_iterators/README.md) · 后：[第 7 章 内部可变性](../chapter07_interior_mutability/README.md) · 原书目录：[本书目录 § 第 6 章](../本书目录.md#第-6-章--基本类型续)

**本章定位**：延续第 4 章 — **`NonZero*` niche** · **`bool.then`** 链式 · **`char` 互转** · **`str` 边界 + Pattern/Searcher + TwoWay** · **切片 `swap`/排序 unsafe 提效** — 证明高级特性来自 **类型系统 + Trait + 安全 unsafe 封装**，非编译器魔术。

**原书主线（已写入）**：6.1 NonZero · 6.2 then/then_some · 6.3 字符转换 · 6.4 is_char_boundary/Pattern/TwoWay · 6.5 copy_nonoverlapping。

**阅读顺序**：**6.1 → 6.2 → … → 6.5**

---


<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **6.1** | 整数类型 | [笔记](./6.1-integer-types.md) |
| **6.2** | 布尔类型 | [笔记](./6.2-bool-type.md) |
| **6.3** | 字符类型 | [笔记](./6.3-char-type.md) |
| **6.4** | 字符串类型 | [笔记](./6.4-string-type.md) |
| **6.5** | 切片类型 | [笔记](./6.5-slice-type.md) |

<!-- /AUTO:SECTION-INDEX -->
## 子节索引

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **6.1** | 整数类型 | ✅ NonZero niche |
| **6.2** | 布尔类型 | ✅ then/then_some |
| **6.3** | 字符类型 | ✅ 互转/escape |
| **6.4** | 字符串类型 | ✅ Pattern/TwoWay |
| **6.5** | 切片类型 | ✅ swap/sort unsafe |

---

## 与主线对照

| 本章 | 本仓库延伸 |
|------|------------|
| 与第 4 章衔接 | [4.2 基本类型分析](../chapter04_primitive_types/README.md)（4.2.1 整数等） |
| 布局 / 表示 | [RFR Ch02 Types](../../02-RFR/Chapter-02-Types/README.md) |
| `str` / `String` | [Book 08 字符串](../../00-Book/08-common-collections/) |

---

## HFT 阅读提示

| 节 | 实盘关联 |
|----|----------|
| **6.1** | 定点价格、tick size、溢出与 wrapping |
| **6.4～6.5** | 符号解析、`&str` 视图 vs 拥有 `String`；行情字段零拷贝借用 |
