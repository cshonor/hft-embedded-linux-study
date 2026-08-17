# M5 · Advanced Standards（进阶标准）

> **里程碑定位：** ⚪ 可选 · 用到再查
> **学习顺序：** M4 之后（14 HFT 主线跑通后）
> **难度：** ⭐⭐⭐⭐

## 包含的书

| 目录 | 书 | 状态 |
|------|-----|------|
| [01-C++17-The-Complete-Guide](./01-C++17-The-Complete-Guide/) | C++17（35 章） | 整章 README 已写 |
| [02-C++20-The-Complete-Guide](./02-C++20-The-Complete-Guide/) | C++20（24 章） | 整章 README 已写 |

## 怎么读

这一层是"新标准特性集"，不是"必读教材"。理解概念即可，工程用到哪个特性查哪章。

### C++17 新手精简路线（8 章）

| 章 | 特性 | 优先级 |
|----|------|--------|
| ch01 | 结构化绑定 | ⭐⭐⭐ |
| ch02 | if/switch 带初始化 | ⭐⭐⭐ |
| ch10 | if constexpr | ⭐⭐⭐ |
| ch15 | optional | ⭐⭐⭐ |
| ch09 | CTAD | ⭐⭐ |
| ch16 | variant | ⭐⭐ |
| ch19 | string_view | ⭐⭐ |
| ch06 | Lambda 扩展 | ⭐⭐ |

新手跳过：ch11 折叠表达式、ch12 字符串模板参数、ch13 auto 模板参数、ch29 PMR、ch32 launder。

### C++20 四大件

| 特性 | 章 | 要点 |
|------|-----|------|
| Concepts | ch03–05 | 编译期类型约束，替代 SFINAE |
| Ranges | ch06–08 | 惰性视图组合 `v \| filter \| transform` |
| Coroutines | ch14–15 | `co_await`/`co_yield`/`promise_type` |
| Modules | ch16 | 替代 `#include`，预编译 BMI |

## C++17 → C++20 依赖

C++20 很多特性在 C++17 基础上完善：先吃透 17 再读 20 更顺。

## 小节笔记

当前为整章 README 粒度。如需按小节拆分，等读到时再拆。

## 跨模块参考

- **04 STL 源码剖析** 物理上在 [M3](../M3-engineering-standards/04-STL-Source-Analysis/)（和 03 Effective STL 一起作工具书）。LEARNING-PATH 提到"如 M3 未读可放 M4"——指的是阅读时机可后移到 M4 阶段，书的物理位置不动。

---

← [学习路线](../LEARNING-PATH.md) · [上一站 M4](../M4-deep-principles/)
