# 第 3 章：接口设计 (Designing Interfaces)

> **Rust for Rustaceans** · [全书目录](../RFR-本书目录.md)  
> 如何设计**地道且难误用**的专业级 API；四条准则贯穿全章。

## 本章结构（与原书对齐）

**5 个主节** · 连同二级子节共 **18 个部分**（4 个带子的主节标题 + 4 + 4 + 2 + 3 + Summary）。

| 主节 | 英文 | 子节 / 笔记 |
|------|------|-------------|
| **1** | Unsurprising | [01 命名](./01-naming-practices.md)（[01-1~5 五系列](./01-1-as-series.md) · [demo](./naming-series-demo/)）· [02 通用 Trait](./02-common-traits-for-types.md)（[02-1 完整解读](./02-1-common-traits-full-guide.md)）· [03 人体工程学 impl](./03-ergonomic-trait-implementations.md)（[03-1 Blanket](./03-1-ergonomic-blanket-full-guide.md) · [03-2 ?Sized](./03-2-question-sized.md) · [demo](./blanket-trait-demo/)）· [04 包装类型](./04-wrapper-types.md) |
| **2** | Flexible | [05 泛型参数](./05-generic-arguments.md) · [06 对象安全](./06-object-safety.md) · [07 借用 vs 拥有](./07-borrowed-vs-owned.md) · [08 可失败与阻塞析构](./08-fallible-blocking-destructors.md) |
| **3** | Obvious | [09 文档](./09-documentation.md) · [10 类型系统引导](./10-type-system-guidance.md) |
| **4** | Constrained | [11 类型演进](./11-type-modifications.md) · [12 Trait 实现控制](./12-trait-implementations.md) · [13 隐藏契约](./13-hidden-contracts.md) |
| **5** | Summary | [14 小结](./14-summary.md) |

## 四条准则（速记）

```text
Unsurprising  → 符合社区直觉，猜也能猜对
Flexible      → 少强加约束，把选择权留给调用方
Obvious       → 类型 + 文档让正确用法最省力
Constrained   → 收紧公开承诺，保留演进空间
```

## 与 The Book / ER 对照

| 主题 | 本仓库 |
|------|--------|
| 方法、`impl` | [5.3 方法语法](../../00-Book/05-structs/5.3-方法语法.md) |
| trait 对象 | [17.2 trait 对象](../../00-Book/17-oop/17.2-为使用不同类型的值而设计的trait对象.md) |
| newtype | [ER Item 06](../../01-ER/Chapter-01-Types/Item-06-newtype-pattern/README.md) |
| 文档 | [ER Item 27](../../01-ER/Chapter-05-Tooling/Item-27-document-public-api/README.md) |
| 可见性 | [ER Item 22](../../01-ER/Chapter-04-Dependencies/Item-22-minimize-visibility/README.md) |

## 旧版单文件

见 git 中的 `3-接口设计-Designing-Interfaces-深度解析.md`；内容已拆入上表。
