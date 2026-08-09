# 第 1 章 STL 概览

**STL Overview**

## 本章讲什么

STL 的伟大不在于"提供了哪些容器"，而在于它的**六大组件**架构与**泛型编程**思想。本章从顶层俯瞰 STL 的组件关系与 SGI/GNU 源码组织，为后续逐章剖析源码建立全景。

## 要点

### 六大组件

| 组件 | 作用 | 示例 |
|------|------|------|
| **容器** | 数据结构 | `vector`/`list`/`map` |
| **算法** | 操作数据的函数 | `sort`/`find`/`copy` |
| **迭代器** | 容器与算法的桥梁 | `iterator`/`traits` |
| **仿函数** | 行为可定制的策略 | `less<T>`/自定义 |
| **适配器** | 改造接口 | `stack`/`bind` |
| **配置器** | 内存分配 | `allocator` |

### 泛型编程的核心思想

STL 把"数据结构"与"算法"解耦——算法不直接操作容器，而是通过**迭代器**这一抽象层操作元素。这让一个 `sort` 能对任意支持随机访问迭代器的容器工作。迭代器的**分类（category）**决定了算法可用的范围，这是通过 `traits` 技巧在编译期分派的。

### SGI STL 源码组织

侯捷剖析的是 SGI STL（GNU C++ 标准库的基础）。关键目录：
- `<stl_alloc.h>`：空间配置器
- `<stl_vector.h>`/`<stl_list.h>`：容器实现
- `<stl_algo.h>`：算法实现
- `<stl_iterator.h>`：迭代器与 traits
- `<stl_function.h>`：仿函数与适配器

## HFT 关联

读 STL 源码的价值在于理解**底层内存模型与复杂度保证**——HFT 选容器、调性能时，知道 `vector` 扩容翻倍、`deque` 分段连续、`hashtable` 开链，才能精准预测延迟。源码级理解是"从会用 STL 到会调优 STL"的门槛。

## 自测题

1. STL 六大组件是什么？它们如何通过迭代器解耦容器与算法？
2. 迭代器分类为什么能在编译期决定算法可用范围？
3. 侯捷剖析的是哪个版本的 STL？它是哪个标准库的基础？

## 代码自测

### Q1: 六大组件协作
```cpp
// STL 六大组件协作示例
template<typename T, typename Alloc = std::allocator<T>>
class vector {  // 容器
    Alloc alloc;  // 分配器
public:
    T* data;
    void push_back(const T& val) {  // 用分配器分配内存
        T* p = alloc.allocate(1);
        std::allocator_traits<Alloc>::construct(alloc, p, val);
    }
};

std::sort(v.begin(), v.end());  // 算法通过迭代器操作容器
```
> 容器如何使用分配器？算法如何与容器解耦？

<details>
<summary>答案与复习指引</summary>

**容器与分配器**：容器是模板参数 `Alloc`，默认 `std::allocator<T>`。容器内所有内存操作通过分配器（`allocate`/`deallocate`/`construct`/`destroy`），不直接调 `new`/`delete`。自定义分配器可接 mempool/hugepage。

**算法与容器解耦**：算法只接收迭代器，不认识容器。`sort(begin, end)` 通过迭代器读写元素，不关心数据存在 vector 还是 array。迭代器是容器和算法之间的**桥梁**。

```
容器 → 迭代器 → 算法
  ↑               ↑
分配器          仿函数/适配器
```

**复习：** → [六大组件](./README.md)
</details>
