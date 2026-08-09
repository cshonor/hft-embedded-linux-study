# 前言 Preface

## 本篇讲什么

Nicolai Josuttis 的前言，介绍 C++17 的定位、本书的读者对象、以及与《C++17 - The Complete Guide》同系列其他书（C++20、C++23）的关系。

## 要点

### C++17 的定位

C++17 不是革命性版本（像 C++11 那样），而是**演进性**版本——大量中小改进的集合。没有"一个巨型特性"（如 C++11 的移动语义、C++20 的 Concepts/Ranges/Modules），但有数十个实用特性让代码更简洁、更安全、更快。

### 核心价值

- **泛型编程**：`if constexpr`、折叠表达式、`auto` 模板参数让模板代码大幅简化。
- **类型安全**：`optional`/`variant`/`any`/`byte`/`string_view` 替代裸指针和 union。
- **性能**：`to_chars`/`from_chars`、PMR、并行 STL、强制拷贝省略。
- **工程化**：`[[nodiscard]]`、`scoped_lock`、CTAD、结构化绑定减少样板代码。

### 读者对象

- 已熟悉 C++11/14 的中高级 C++ 程序员。
- 想了解"C++17 到底加了什么、怎么用、有什么坑"。
- 库作者和需要写泛型代码的人（C++17 对泛型支持大改进）。

### 与同系列的关系

Josuttis 的 C++ 系列三本：
1. **C++17 - The Complete Guide**（本书）：C++17 全特性。
2. **C++20 - The Complete Guide**（[第 10 本](../../02-C++20-The-Complete-Guide/)）：C++20 的 Concepts/Ranges/Modules/Coroutines。
3. C++23 - The Complete Guide：C++23 增量。

阅读顺序建议：先 C++17 再 C++20。C++20 的很多特性建立在 C++17 之上（如 Concepts 约束 CTAD、Ranges 延续 string_view 思路、`std::span` 补充 string_view）。

### Josuttis 的风格

- **务实**：每个特性都有"什么时候用、什么时候不用"的建议。
- **示例丰富**：大量小代码片段，不用大例子。
- **陷阱提示**：标注常见误用和兼容性问题。

## HFT 关联

- **C++17 是 HFT 的实用基线**：很多 HFT 项目用 C++17（不一定升到 C++20/23），因为 17 的特性集已经覆盖了 string_view、optional、variant、PMR、to_chars 等核心需求。
- **演进而非革命**：HFT 代码库大且对稳定性敏感，C++17 的"中小改进"风格适合渐进迁移，不像 C++20 的 Modules/Ranges 那样需要大改。
- **作者务实风格契合 HFT**：HFT 关心"能不能用、有什么代价"，Josuttis 的特性评估视角与 HFT 一致。
- **与 02 的衔接**：读完 01 再读 02-C++20，理解 C++20 如何在 17 基础上扩展（如 Concepts 约束模板、Ranges 替代迭代器、Coroutines 替代回调）。

## 自测题

1. C++17 是革命性版本还是演进性版本？它的特点是什么？
2. C++17 的核心价值分哪几类？
3. Josuttis 的 C++ 系列三本是什么？阅读顺序？
4. 为什么 C++17 适合 HFT 作为实用基线？
5. C++20 如何建立在 C++17 之上？举例（Concepts 约束 CTAD 等）。

## 代码自测

### Q1: C++17 定位
```cpp
// C++14
auto x = 42;  // auto 推导
// C++17 新增
auto [a, b] = std::make_pair(1, 2.0);  // 结构化绑定
if (auto it = m.find(k); it != m.end()) {}  // if-init
std::optional<int> find(int key);  // optional
```
> C++17 相比 C++14 的核心改进方向是什么？为什么说它是"实用主义"版本？

<details>
<summary>答案与复习指引</summary>

C++17 的核心方向：
1. **简化语法**：结构化绑定、if-init、constexpr if、fold expressions
2. **新类型**：optional、variant、any、string_view
3. **标准库扩展**：filesystem、parallel STL、pmr
4. **编译期增强**：constexpr if、CTAD、inline variables

"实用主义"：没有颠覆性概念（如 C++11 的 lambda/move 或 C++20 的 concepts/ranges），而是填补日常痛点——让代码更简洁、更安全。很多特性是对 C++11/14 的"打磨"。

**复习：** → [C++17 概览](./README.md)
</details>
