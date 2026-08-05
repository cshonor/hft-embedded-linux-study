# 第 3 章 迭代器与 traits

**Iterators and Traits**

## 本章讲什么

迭代器是 STL 解耦容器与算法的关键抽象。本章讲迭代器的五种分类、与之关联的五个关联类型（associated types），以及用 `traits` 技巧在**编译期萃取类型信息**的机制——这是 C++ 模板元编程的启蒙。

## 要点

### 五种迭代器分类

| 分类 | 能力 | 标签 |
|------|------|------|
| 输入迭代器 | 只读、单遍、`++` | `input_iterator_tag` |
| 输出迭代器 | 只写、单遍、`++` | `output_iterator_tag` |
| 前向迭代器 | 读写、多遍、`++` | `forward_iterator_tag` |
| 双向迭代器 | + `--` | `bidirectional_iterator_tag` |
| 随机访问迭代器 | + `[]`、`+`/`-` | `random_access_iterator_tag` |

标签（tag）用**继承**构成层级（`forward` 继承 `input` 等），让算法能按"最具体匹配"分派。

### 五个关联类型

迭代器需内嵌定义：`value_type`/`difference_type`/`pointer`/`reference`/`iterator_category`。算法靠这些类型声明临时变量、计算距离、选择实现。

### traits 萃取机制

原生指针（如 `int*`）无法内嵌 typedef，所以 SGI 用 `iterator_traits` 模板**偏特化**萃取：

```cpp
template<class I> struct iterator_traits { typedef typename I::value_type value_type; };
template<class T> struct iterator_traits<T*> { typedef T value_type; };          // 原生指针偏特化
template<class T> struct iterator_traits<const T*> { typedef T value_type; };    // const 指针偏特化
```

`traits` 是"问类型问题"的统一接口——算法写 `typename iterator_traits<Iter>::value_type` 就能同时处理类迭代器和原生指针。这个技巧后来演化为 `type_traits`（C++11 `<type_traits>`）。

### 编译期分派

算法用 `iterator_category` 标签 + 函数重载，在编译期选最优实现：

```cpp
template<class Iter, class Dist>
void advance_impl(Iter& it, Dist n, input_iterator_tag) { while(n--) ++it; }     // 通用
template<class Iter, class Dist>
void advance_impl(Iter& it, Dist n, random_access_iterator_tag) { it += n; }     // 随机访问 O(1)
```

`std::advance` 据迭代器分类在编译期选 O(1) 或 O(n) 实现——零运行开销分派。

## HFT 关联

- **traits 是 C++ 元编程根基**：`type_traits`（`is_trivially_copyable`/`is_pod`）让 HFT 代码在编译期选 `memcpy` 或逐元素构造，零运行开销。
- **编译期分派换性能**：`copy` 对 `random_access_iterator` 用 `memmove`、对 `input_iterator` 用逐元素拷贝——这种按能力分派是 HFT "编译期策略选择"的原型。
- **`iterator_category` 决定算法复杂度**：热路径选容器时，迭代器分类决定可用算法的效率上限——`vector` 随机访问让 `sort` 走内省排序。

## 自测题

1. 五种迭代器分类的层级关系（继承）是什么？标签继承如何帮助算法分派？
2. 为什么原生指针需要 `iterator_traits` 偏特化？traits 解决了什么问题？
3. `std::advance` 如何在编译期按迭代器分类选 O(1) 或 O(n) 实现？
4. traits 技巧后来演化为 C++11 的什么机制？HFT 用它做什么编译期决策？
5. 迭代器分类如何决定 STL 算法（如 `sort`/`copy`）的实现选型？
