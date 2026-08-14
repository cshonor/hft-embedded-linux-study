# 第 2 章：类型 (Types)

> **Rust for Rustaceans** · [全书目录](../RFR-本书目录.md)  
> 原书章名：**Types** — 从内存表示到 Trait 分发、存在类型，建立「零成本抽象」在机器层面的直觉。

## 本章结构（与原书对齐）

**4 个主节** · 连同二级子节共 **13 个部分**（2 个带子的主节标题 + 4 + 5 + 1 + 1）。

| 主节 | 英文 | 子节 / 笔记 |
|------|------|-------------|
| **1** | Types in Memory | [01 对齐](./01-alignment.md) · [02 布局](./02-layout.md) · [layout-demo](./layout-demo/) · [03 复合类型](./03-complex-types.md) · [04 DST 与宽指针](./04-dst-wide-pointers.md)（[04.1](./04-1-dst-basics.md)～[04.4](./04-4-containers-ffi-hft.md)） |
| **2** | Traits and Trait Bounds | [05 编译与分发](./05-compilation-dispatch.md)（[05.1](./05-1-static-vs-dynamic.md)～[05.4](./05-4-selection-hft.md)）· [06 泛型 Trait](./06-generic-traits.md)（[06.1](./06-1-associated-vs-generic.md)～[06.2](./06-2-existential-hft.md)）· [07 相干性与孤儿规则](./07-coherence-orphan-rule.md)（[07.1](./07-1-orphan-rule.md)～[07.3 Newtype](./07-3-newtype-practice.md) · [orphan-rule-demo](./orphan-rule-demo/)）· [08 Trait 限定](./08-trait-bounds.md)（[08.1](./08-1-syntax-static-dynamic.md)～[08.3](./08-3-examples-pitfalls.md) · [trait-bounds-demo](./trait-bounds-demo/)）· [09 标记 Trait](./09-marker-traits.md)（[09.1](./09-1-marker-basics.md)～[09.4](./09-4-unsafe-impl.md)） |
| **3** | Existential Types | [10 存在类型](./10-existential-types.md)（[10.1](./10-1-logic-positions.md)～[10.3](./10-3-limits-selection.md)） |
| **4** | Summary | [11 小结](./11-summary.md) |

## 阅读顺序

```text
01 → 02 → 03 → 04 → 04.1 → 04.2 → 04.3 → 04.4 → 05 → 05.1 → 05.2 → 05.3 → 05.4 → 06 → 06.1 → 06.2 → 07 → 07.1 → 07.2 → 07.3 → 08 → 08.1 → 08.2 → 08.3 → 09 → 09.1 → 09.2 → 09.3 → 09.4 → 10 → 10.1 → 10.2 → 10.3 → 11
```

## 与 The Book / ER / 06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17 对照

| 主题 | 本仓库 |
|------|--------|
| 泛型、trait | [10.1 泛型](../../00-Book/10-generics-traits-lifetimes/10.1-泛型数据类型.md) · [10.2 trait](../../00-Book/10-generics-traits-lifetimes/10.2-trait.md) |
| trait 对象 | [17.2 trait 对象](../../00-Book/17-oop/17.2-为使用不同类型的值而设计的trait对象.md) |
| `impl Trait` | [10.2.3 impl Trait 全解](../../00-Book/10-generics-traits-lifetimes/10.2.3-impl-Trait全解.md) |
| 静/动态分发 | [ER Item 12](../../01-ER/Chapter-01-Types/Item-12-generics-vs-trait-objects/README.md) |
| 布局 / unsafe | [RFR 第 9 章](../Chapter-09-Unsafe-Code/9-不安全代码-Unsafe-Code-深度解析.md) · [06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17 ch05](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/part02_src_to_machine/chapter05_ir_advanced_type/README.md) |

## 旧版单文件

早期合并稿已拆入上表；历史版本见 git 中的 `2-类型-Types-深度解析.md`。
