# 引言 Introduction

## STL 是什么

STL（Standard Template Library）是 C++ 标准库的核心——容器、迭代器、算法、函数对象四件套，以**泛型模板**贯通。它不是"一堆工具的集合"，而是一套**约定**：容器提供迭代器，算法通过迭代器操作容器，函数对象定制算法行为。理解这套约定比记 API 更重要。

## 为什么单独学 Effective STL

STL 的设计哲学与手写循环完全不同——它鼓励你用"算法 + 迭代器"表达意图，而非"循环 + 下标"。但这种范式转变有一批固有陷阱：迭代器失效、`remove` 不真正删除、等价与相等、仿函数的值语义……Effective STL 这本书专门讲这些"用对 STL"的细节。

## 全书结构

| 章 | 主题 | item 数 |
|----|------|---------|
| 1 | 容器 | 11 |
| 2 | vector 和 string | 6 |
| 3 | 关联容器 | 8 |
| 4 | 迭代器 | 6 |
| 5 | 算法 | 6 |
| 6 | 仿函数 | 6 |
| 7 | 用 STL 编程 | 7 |

全书 50 条 item。笔记按章节整理，重点提炼"为什么"与"坑在哪"。

## HFT 视角

STL 是 HFT C++ 引擎的默认工具箱，但有性能纪律：热路径避免 `map`/`unordered_map` 的逐元素插入（预 `reserve`）、避免 `string` 频繁分配（用 `string_view`）、用 `vector` 连续存储换取 cache 友好。Effective STL 的很多 item 直接对应 HFT 的性能与正确性铁律。

## 自测题

1. STL 的四大组件是什么？它们如何通过"约定"协作？
2. 为什么"算法 + 迭代器"范式比"循环 + 下标"更值得学？
3. HFT 热路径用 STL 容器时，最重要的三条性能纪律是什么？

## 代码自测

### Q1: STL 六大组件关系
```cpp
// 六大组件协作示例
std::vector<int> v = {3, 1, 4, 1, 5, 9};  // 容器
std::sort(v.begin(), v.end());             // 算法 + 迭代器
auto it = std::find_if(v.begin(), v.end(),
    std::bind(std::greater<int>(), std::placeholders::_1, 4));  // 仿函数 + 适配器
std::vector<int> result;
std::copy(it, v.end(), std::back_inserter(result));  // 适配器(back_insert_iterator)
```
> 这段代码用到了 STL 的哪几个组件？各起什么作用？

<details>
<summary>答案与复习指引</summary>

| 组件 | 代码中的角色 |
|------|-------------|
| **容器** | `vector<int> v` — 存储数据 |
| **算法** | `sort`、`find_if`、`copy` — 操作数据 |
| **迭代器** | `v.begin()`、`it` — 连接容器与算法 |
| **仿函数** | `greater<int>()` — 比较策略 |
| **适配器** | `bind(...)` 包装 greater、`back_inserter(result)` 适配 copy 输出 |
| **分配器** | `vector` 默认用 `allocator<int>` — 分配内存（隐式） |

六大组件通过迭代器解耦：算法不认识容器，只认识迭代器。

**复习：** → [STL 全景](./README.md)
</details>
