# 3.2 五个关联类型

> 第 3 章 迭代器与 traits · 第 2 节 · 上一节：[3.1 迭代器分类](01-iterator-categories.md) · 下一节：[3.3 traits 萃取机制](03-traits-mechanism.md)

## 为什么要学这个（先建立直觉）

在 C 里，指针指向什么类型是显式的——`int*` 指向 int，`char*` 指向 char。但 C++ 泛型算法接收的是模板参数 `Iter`，它需要知道"这个迭代器指向什么类型"来声明临时变量。

```c
/* C: 类型直接写 */
int sum(int* begin, int* end) {
    int total = 0;  // 类型已知：int
    while (begin != end) total += *begin++;
    return total;
}
```

```cpp
// C++: 模板参数 Iter，不知道指向什么类型
template<typename Iter>
void sum(Iter begin, Iter end) {
    ??? total = 0;  // 这里写什么类型？
    while (begin != end) total += *begin++;
}
```

**直觉**：迭代器需要内嵌"我指向什么类型"的信息，让算法能推导出正确的临时变量类型。

## 这节讲什么

### 五个关联类型

```cpp
template<typename T>
struct iterator {
    using iterator_category = ...;  // 1. 迭代器分类（Input/Forward/.../RandomAccess）
    using value_type = ...;         // 2. 迭代器指向的值类型
    using difference_type = ...;    // 3. 两个迭代器之间的距离类型（通常 ptrdiff_t）
    using pointer = ...;            // 4. 指向 value_type 的指针
    using reference = ...;          // 5. value_type 的引用
};
```

### 每个类型的用途

```cpp
template<typename Iter>
void algorithm(Iter first, Iter last) {
    // 1. iterator_category: 编译期分派（见 3.4 节）
    using cat = typename std::iterator_traits<Iter>::iterator_category;

    // 2. value_type: 声明临时变量
    using val_t = typename std::iterator_traits<Iter>::value_type;
    val_t temp = *first;  // 需要知道值类型

    // 3. difference_type: 计算距离
    using diff_t = typename std::iterator_traits<Iter>::difference_type;
    diff_t n = std::distance(first, last);  // 需要距离类型

    // 4. pointer / 5. reference: 声明指针/引用
    using ref_t = typename std::iterator_traits<Iter>::reference;
    ref_t ref = *first;  // 可能是 T& 或 proxy type
}
```

### STL 容器中的定义

```cpp
// vector<int>::iterator 的关联类型
template<typename T>
class vector_iterator {
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;
    // 实际上 vector::iterator 就是 T*，这些类型由 iterator_traits<T*> 萃取
};

// list<int>::iterator 的关联类型
template<typename T>
class list_iterator {
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;
};
```

### C++20: iterator_concept 替代

C++20 引入 Concepts 后，迭代器关联类型可由 `std::iter_value_t`/`std::iter_reference_t` 等直接萃取，不再要求迭代器内嵌 typedef。

## 常见错误（新手踩坑）

### 错误 1：自定义迭代器忘了定义关联类型

```cpp
struct MyIter {
    int* ptr;
    int& operator*() { return *ptr; }
    MyIter& operator++() { ++ptr; return *this; }
    // 忘了 value_type/iterator_category 等！
    // std::iterator_traits<MyIter> 无法萃取 → 算法编译失败
};
```

### 错误 2：value_type 写成引用

```cpp
struct BadIter {
    using value_type = int&;  // 错！value_type 应该是 int，不是 int&
    // value_type 表示"值的类型"，不是"引用的类型"
    // reference 才是 int&
};
```

### 错误 3：difference_type 用 int

```cpp
struct BadIter {
    using difference_type = int;  // 可能溢出！
    // 大容器（> 2^31 元素）距离会溢出
    // 应该用 std::ptrdiff_t
};
```

## 新手要点（和 C 的区别）

| 方面 | C | C++ STL |
|------|---|---------|
| 值类型来源 | 直接看指针类型 `int*` | 迭代器内嵌 `value_type` typedef |
| 距离类型 | int/size_t | `difference_type`（通常 ptrdiff_t） |
| 分类表达 | 无 | `iterator_category` tag |
| 类型萃取 | 无 | `iterator_traits` 模板 |

## HFT 关联

- **value_type 决定临时变量类型**：算法内部用 `value_type` 声明临时变量，选对类型避免截断
- **difference_type 影响大数组**：HFT 处理百万级数据时，difference_type 必须是 64 位（ptrdiff_t）
- **C++20 简化**：`std::iter_value_t<Iter>` 比内嵌 typedef 更方便，减少自定义迭代器的样板代码

## 代码自测

### Q1: value_type 的用途

```cpp
template<typename Iter>
auto my_accumulate(Iter first, Iter last) {
    using T = typename std::iterator_traits<Iter>::value_type;
    T sum = T();  // 需要知道值类型才能声明 sum
    for (; first != last; ++first) sum += *first;
    return sum;
}
```
> 如果不写 value_type，sum 的类型怎么确定？

<details>
<summary>答案</summary>

无法确定。算法模板不知道 `Iter` 指向什么类型，必须通过 `value_type` 萃取。

替代方案（C++14+）：
```cpp
// 用 decltype 推导
auto sum = *first; ++first;  // 初始化为第一个元素
// 但空范围时 first == last，*first 是 UB

// 或用 auto + 初始值
template<typename Iter, typename T>
T my_accumulate(Iter first, Iter last, T init) {
    for (; first != last; ++first) init += *first;
    return init;
}
// 这就是 std::accumulate 的设计——要求用户传初始值
```

`value_type` 的价值：让算法能自己声明正确类型的临时变量，不依赖用户传初始值。
</details>

### Q2: 原生指针的关联类型

```cpp
int arr[] = {1, 2, 3};
int* p = arr;
// int* 的五个关联类型是什么？
```

<details>
<summary>答案</summary>

通过 `iterator_traits<int*>` 偏特化萃取：

```cpp
template<typename T>
struct iterator_traits<T*> {
    using iterator_category = std::random_access_iterator_tag;  // 指针是随机访问
    using value_type = T;          // int
    using difference_type = std::ptrdiff_t;  // 指针距离用 ptrdiff_t
    using pointer = T*;            // int*
    using reference = T&;          // int&
};
```

原生指针天然是 RandomAccessIterator（支持 `*`、`++`、`--`、`+n`、`[]`、`<`）。

**关键**：`iterator_traits<T*>` 的偏特化让 `int*` 能被 STL 算法当作随机访问迭代器使用——这就是 `std::sort(arr, arr+3)` 能工作的原因。
</details>

### Q3: difference_type 溢出

```cpp
std::vector<int> v(3000000000LL);  // 30 亿元素
auto dist = std::distance(v.begin(), v.end());
// dist 的类型是什么？会溢出吗？
```

<details>
<summary>答案</summary>

`dist` 的类型是 `std::ptrdiff_t`（64 位系统上通常是 `long long`），不会溢出。

```cpp
using diff_t = std::iterator_traits<std::vector<int>::iterator>::difference_type;
// diff_t = std::ptrdiff_t = long long (64-bit)
```

如果错误地用 `int`：
```cpp
int dist = std::distance(v.begin(), v.end());  // 30 亿 > INT_MAX (~21 亿) → 溢出！
```

**教训**：迭代器距离永远用 `difference_type`（ptrdiff_t），不要用 int。
</details>

### Q4: 自定义迭代器关联类型

```cpp
template<typename T>
class RingBufferIterator {
    T* ptr;
    T* begin;
    size_t cap;
public:
    // 补全五个关联类型
};
```

<details>
<summary>答案</summary>

```cpp
template<typename T>
class RingBufferIterator {
    T* ptr;
    T* begin;
    size_t cap;
public:
    using iterator_category = std::random_access_iterator_tag;  // 支持随机访问
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    reference operator*() const { return *ptr; }
    pointer operator->() const { return ptr; }
    RingBufferIterator& operator++() { ++ptr; return *this; }
    // ... 其他操作 ...
};
```

五个类型缺一不可：
- `iterator_category`：决定能用哪些算法
- `value_type`：让算法声明临时变量
- `difference_type`：让算法计算距离
- `pointer`/`reference`：让算法声明指针/引用

**C++20 简化**：用 `std::iterator_traits` 特化或 Concepts，不必在类内写 typedef。
</details>

## 参考与延伸

- 上一节：[3.1 迭代器分类](01-iterator-categories.md)
- 下一节：[3.3 traits 萃取机制](03-traits-mechanism.md)
