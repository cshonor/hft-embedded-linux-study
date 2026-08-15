# 第 4 章 · 基本类型及基本 Trait

> 所属：[03 DeepRustStdLib](../README.md) · 前：[第 3 章 内存操作](../chapter03_memory_model/README.md) · 后：[第 5 章 迭代器](../chapter05_iterators/README.md) · 原书目录：[本书目录 § 第 4 章](../本书目录.md#第-4-章--基本类型及基本-trait)

**本章定位**：打破 **`Option`/`Result`/`?` 是编译器魔法** 的定式 — **`core` intrinsics**、数值/空值/错误类型、**Marker / ops / Try / Range / Index** 等 **Trait** 才是语法表现力的来源；读容器与并发前的 **类型与运算符底座**。

**原书主线（已写入）**：4.1 intrinsic 三类 · 4.2 基本类型系统化 · 4.3 Trait 解语法糖 · **宏+泛型+闭包+Trait** 搭建语言表象。

**阅读顺序**：**4.1 → 4.2 → 4.3**

---


<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **4.1** | 固有函数库 | [笔记](./4.1-inherent-fns.md) |
| **4.1.1** | 原子操作函数 | [笔记](./4.1.1-atomic-fns.md) |
| **4.1.2** | 数学函数及位操作函数 | [笔记](./4.1.2-math-bit-fns.md) |
| **4.1.3** | 指令预取优化函数、断言类函数及栈获取函数 | [笔记](./4.1.3-prefetch-assert-stack-fns.md) |
| **4.2** | 基本类型分析 | [笔记](./4.2-basic-types.md) |
| **4.2.1** | 整数类型 | [笔记](./4.2.1-integer-types.md) |
| **4.2.2** | 浮点类型 | [笔记](./4.2.2-float-types.md) |
| **4.2.3** | Option<T> 类型 | [笔记](./4.2.3-option-type.md) |
| **4.2.4** | 引用类型 match 语法研究 | [笔记](./4.2.4-ref-match-syntax.md) |
| **4.2.5** | Result<T, E> 类型 | [笔记](./4.2.5-result-type.md) |
| **4.3** | 基本 Trait | [笔记](./4.3-basic-traits.md) |
| **4.3.1** | 编译器内置 Marker Trait | [笔记](./4.3.1-marker-traits.md) |
| **4.3.2** | 算术运算符 Trait | [笔记](./4.3.2-arithmetic-traits.md) |
| **4.3.3** | `?` 运算符 Trait | [笔记](./4.3.3-try-trait.md) |
| **4.3.4** | 范围运算符 Trait | [笔记](./4.3.4-range-traits.md) |
| **4.3.5** | 索引运算符 Trait | [笔记](./4.3.5-index-traits.md) |

<!-- /AUTO:SECTION-INDEX -->
## 子节索引

### 4.1 固有函数库

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **4.1** | 固有函数库 | ✅ |
| **4.1.1** | 原子操作函数 | ✅ |
| **4.1.2** | 数学函数及位操作函数 | ✅ |
| **4.1.3** | 指令预取优化函数、断言类函数及栈获取函数 | ✅ |

### 4.2 基本类型分析

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **4.2** | 基本类型分析 | ✅ |
| **4.2.1** | 整数类型 | ✅ |
| **4.2.2** | 浮点类型 | ✅ |
| **4.2.3** | `Option<T>` 类型 | ✅ |
| **4.2.4** | 引用类型 match 语法研究 | ✅ |
| **4.2.5** | `Result<T, E>` 类型 | ✅ |

### 4.3 基本 Trait

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **4.3** | 基本 Trait | ✅ |
| **4.3.1** | 编译器内置 Marker Trait | ✅ |
| **4.3.2** | 算术运算符 Trait | ✅ |
| **4.3.3** | `?` 运算符 Trait | ✅ |
| **4.3.4** | 范围运算符 Trait | ✅ |
| **4.3.5** | 索引运算符 Trait | ✅ |

---

## 与主线对照

| 本章 | 本仓库延伸 |
|------|------------|
| 4.1.1 原子操作 | [05-atomic](../../05-Async-Concurrency-Network/01-atomic/README-学习区.md) |
| 4.2.1～4.2.2 整数 / 浮点 | [RFR Ch02 布局与对齐](../../02-RFR/Chapter-02-Types/02-layout.md) |
| 4.2.3 / 4.2.5 `Option` / `Result` | [2.6 错误与 std](../chapter02_rust_features_summary/2.6-error-handling-in-stdlib.md) · [RFR Ch04](../../02-RFR/Chapter-04-Error-Handling/README.md) |
| 4.3.1 Marker Trait | [RFR Ch02 §09](../../02-RFR/Chapter-02-Types/09-marker-traits.md) |
| 4.3.3 `?` / `Try` | [RFR Ch04 传播](../../02-RFR/Chapter-04-Error-Handling/04-propagating-errors.md) |

---

## HFT 阅读提示

| 节 | 实盘关联 |
|----|----------|
| **4.1.1** | 无锁计数、订单序号、行情版本号 |
| **4.1.2** | 位掩码、定点化价格字段 |
| **4.1.3** | 热路径预取、debug 断言与栈深度诊断 |
| **4.3.1** | `Send`/`Sync` 决定行情结构体能否 `Arc` 跨线程 |
