# 前言 Preface

## 本篇讲什么

Josuttis 的前言，介绍 C++20 的定位（"C++11 以来最大的版本"）、四大件（Concepts/Ranges/Modules/Coroutines）的背景、本书的读者对象和阅读建议。

## 要点

### C++20 的定位

C++20 是继 C++11 之后**最大的** C++ 版本更新。C++11 引入了移动语义、智能指针、lambda、并发；C++20 引入了四大件：

1. **Concepts**：模板参数的语义约束，编译错误从天书变人话。
2. **Ranges**：STL 的革命性重构，惰性组合替代手写循环。
3. **Modules**：替代 `#include`，解决编译慢和宏污染。
4. **Coroutines**：可挂起/恢复的函数，异步代码同步写。

加上 `<=>`、`std::format`、`std::span`、`jthread`、日历时区等，C++20 是一次全面升级。

### 四大件的背景

| 特性 | 提出/讨论 | 落地 | 难度 |
|------|-----------|------|------|
| Concepts | C++0x 时代 | C++20（历经 Concepts Lite 等） | 中 |
| Ranges | C++14 Ranges TS | C++20 | 高 |
| Modules | C++15 Modules TS | C++20 | 高 |
| Coroutines | C++14 Coroutines TS | C++20（只有机制，库待 C++23/26） | 极高 |

每个都讨论了十年以上，C++20 终于落地。

### 读者对象

- 已熟悉 C++17 的中高级程序员。
- 想理解 C++20 四大件的原理和用法。
- 库作者和需要写泛型/异步代码的人。

### 阅读建议

1. **Concepts 先行**：ch03-05 是基础，Ranges 和其他特性大量用 Concepts。
2. **Ranges 紧随**：ch06-08 是 STL 的未来，理解视图和组合。
3. **Modules 可跳读**：ch16 工具链不成熟，理解概念即可，实践等编译器/构建系统成熟。
4. **Coroutines 最后**：ch14-15 最难，需要理解 promise_type/awaitable 机制。
5. **小改进穿插**：ch09-13、ch17-24 是实用改进，可按需读。

### 与 01 C++17 的衔接

C++20 建立在 C++17 之上：
- Concepts 约束 C++17 的 CTAD、auto 参数。
- Ranges 延续 C++17 string_view 的"视图"思路。
- `std::format` 补全 C++17 to_chars 之上的格式化层。
- Coroutines 依赖 C++17 的结构化绑定、if constexpr。

先吃透 09 再读 10 更顺。

### Josuttis 的风格

延续 09 的务实风格——每个特性有"什么时候用、什么时候不用、有什么坑"的建议，大量小代码片段。

## HFT 关联

- **C++20 对 HFT 的价值**：`std::format`（快日志）、`std::span`（缓冲视图）、`std::bit`（位操作指令）、`consteval`（编译期表）、`source_location`（日志元信息）——这些是 HFT 立即可用的改进。
- **四大件在 HFT 的采用节奏**：
  - Concepts：立即采用（编译期约束，零运行开销）。
  - Ranges：离线/回测采用，热路径慎用（迭代器包装有开销）。
  - Modules：等工具链成熟（CMake/Ninja 支持完善后）。
  - Coroutines：管理通道采用（异步 IO），热路径不用。
- **`<=>` 默认比较**：HFT 数据结构多，`<=>` default 大幅减少样板代码。
- **`std::format` 替代 iostream**：日志性能 5-10 倍提升。
- **编译器支持**：C++20 特性需要较新编译器（GCC 10+/Clang 10+/MSVC 2019 16.8+），HFT 项目迁移要评估工具链。

## 自测题

1. C++20 的四大件是什么？各自解决什么问题？
2. 为什么说 C++20 是"C++11 以来最大版本"？
3. Concepts 在 C++20 中的基础地位？其他特性如何依赖它？
4. HFT 对 C++20 四大件的采用节奏分别是什么？为什么 Modules 要等？
5. C++20 建立在 C++17 之上的例子有哪些？
