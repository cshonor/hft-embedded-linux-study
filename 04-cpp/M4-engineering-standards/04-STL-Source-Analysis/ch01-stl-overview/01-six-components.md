# 1.1 STL 六大组件

> 第 1 章 STL 概览 · 第 1 节 · 上一节：[本章概览](README.md) · 下一节：[1.2 泛型编程](02-generic-programming.md)

## 为什么要学这个（先建立直觉）

在 C 里，数据结构和算法是"硬编码"的——`qsort` 只能排序数组，`bsearch` 只能二分查找数组。没有"通用算法适配多种容器"的概念。

```c
/* C: 每种数据结构配一套专用操作 */
int arr[] = {3, 1, 4};
qsort(arr, 3, sizeof(int), cmp);  // 只能排数组

struct Node { int val; struct Node* next; };
// 链表排序？手写归并，完全不同的代码
// 哈希表查找？另一套代码
```

```cpp
// C++ STL: 一个 sort 通吃所有支持随机访问迭代器的容器
std::vector<int> v = {3, 1, 4};
std::sort(v.begin(), v.end());

int arr[] = {3, 1, 4};
std::sort(arr, arr + 3);  // 同一个 sort！

// 链表用成员 sort（因为迭代器类型不同）
std::list<int> l = {3, 1, 4};
l.sort();
```

**直觉**：STL 用六大组件解耦了"数据怎么存"和"数据怎么操作"，让算法和容器自由组合。

## 这节讲什么

### 六大组件总览

| 组件 | 作用 | C 中对应 | 示例 |
|------|------|---------|------|
| **容器** | 存储数据的数据结构 | 数组/链表/手写哈希 | `vector`/`list`/`map` |
| **算法** | 操作数据的函数模板 | `qsort`/`bsearch`/手写 | `sort`/`find`/`copy` |
| **迭代器** | 容器与算法的桥梁 | 指针 | `begin()`/`end()` |
| **仿函数** | 策略对象（比较/运算） | 函数指针 | `less<T>`/lambda |
| **适配器** | 改造接口 | 无 | `stack`/`back_inserter` |
| **配置器** | 内存分配策略 | `malloc`/`free` | `allocator<T>` |

### 组件协作关系

```
        容器 ──持有──→ 数据
         │                ↑
    分配器               │
    (allocator)     算法通过迭代器
         │           读写数据
    管理内存              │
                  仿函数定制策略
                  适配器改造接口
```

**核心解耦**：算法不认识容器，只认识迭代器。`sort(begin, end)` 通过迭代器读写元素，不关心数据存在 vector 还是 array 里。

### 源码中的组件对应

```cpp
// 容器：vector<T, Alloc> 内部用分配器
template<typename T, typename Alloc = std::allocator<T>>
class vector {
    Alloc alloc;  // 配置器组件
public:
    T* data;      // 数据存储
    // 迭代器：vector 的迭代器就是 T*
    T* begin() { return data; }
    T* end() { return data + size_; }
};

// 算法：sort 只接收迭代器，不认识 vector
template<typename RandomAccessIter>
void sort(RandomAccessIter first, RandomAccessIter last);

// 仿函数：定制排序策略
std::sort(v.begin(), v.end(), std::greater<int>{});  // 降序

// 适配器：stack 包装 deque
std::stack<int> s;  // = std::stack<int, std::deque<int>>
```

## 常见错误（新手踩坑）

### 错误 1：以为算法直接操作容器

```cpp
// 错误认知：sort(vector) 直接操作 vector
// 实际：sort(begin, end) 只通过迭代器操作
std::vector<int> v;
// std::sort(v);  // 错！sort 不接收容器，接收迭代器
std::sort(v.begin(), v.end());  // 对
```

### 错误 2：混淆仿函数和函数指针

```cpp
// 函数指针：不能内联，不能携带状态
bool cmp(int a, int b) { return a < b; }

// 仿函数：可内联，可携带状态
struct Cmp {
    int threshold;
    bool operator()(int a, int b) const { return a < b; }
};
```

### 错误 3：不知道配置器的存在

```cpp
// 大多数人不知道 vector 有第二个模板参数 Alloc
std::vector<int> v;
// 等价于 std::vector<int, std::allocator<int>> v;
// allocator 决定了内存怎么分配
```

## 新手要点（和 C 的区别）

| 方面 | C | C++ STL |
|------|---|---------|
| 数据结构 | 手写或库特定 | 容器组件（统一接口） |
| 算法 | 与数据结构绑定 | 与容器解耦（通过迭代器） |
| 策略 | 函数指针 | 仿函数/lambda（可内联） |
| 内存 | malloc/free | 配置器（可定制） |

## HFT 关联

- **自定义 allocator 接 mempool**：`std::vector<T, MempoolAlloc<T>>` 让容器走预分配池，热路径零 malloc
- **算法解耦换复用**：同一份 `lower_bound` 代码可用于 vector/array/自定义容器，减少重复代码
- **迭代器分类决定性能**：vector 随机访问迭代器让 sort 走内省排序，list 双向迭代器只能走归并

## 代码自测

### Q1: 组件识别

```cpp
std::map<int, std::string> m;
m[1] = "hello";
auto it = m.find(1);
std::priority_queue<int> pq;
```
> 上面代码用到了哪几个 STL 组件？

<details>
<summary>答案</summary>

- **容器**：`map`（关联容器）、`priority_queue`（容器适配器）
- **迭代器**：`it`（map::iterator，双向迭代器）
- **配置器**：`map` 内部默认用 `std::allocator`（虽然没显式写）
- **仿函数**：`map<int, string>` 默认用 `std::less<int>` 做键比较；`priority_queue` 默认用 `std::less<int>` 做堆序

`find` 是成员函数（非通用算法），`m[1]` 是容器接口。
</details>

### Q2: 算法与容器解耦

```cpp
std::vector<int> v = {3, 1, 4};
int arr[] = {3, 1, 4};
std::sort(v.begin(), v.end());
std::sort(arr, arr + 3);
```
> 为什么同一个 `std::sort` 能排 vector 和 C 数组？

<details>
<summary>答案</summary>

因为 `sort` 只接收迭代器，不认识容器。

- `v.begin()` 返回 `int*`（vector 迭代器是原生指针）
- `arr` 也是 `int*`（数组名退化为指针）

两者类型相同（`int*`），所以实例化同一个 `sort<int*>` 模板。

这就是 STL 的核心设计：**算法只认迭代器接口，不关心底层是容器还是数组**。
</details>

### Q3: 配置器的作用

```cpp
std::vector<int> v1;                              // A: 默认 allocator
std::vector<int, MempoolAlloc<int>> v2(alloc);   // B: 自定义 allocator
```
> A 和 B 在内存分配上有什么区别？

<details>
<summary>答案</summary>

- **A（默认）**：内部用 `std::allocator<int>`，底层走 `operator new`/`operator delete` = `malloc`/`free`
- **B（自定义）**：内部用 `MempoolAlloc<int>`，你控制的内存池分配/回收

自定义 allocator 的价值：HFT 热路径避免 malloc 的锁竞争和碎片，走预分配内存池。

**注意**：C++11 起 allocator 要求无状态才能等价（同一类型的两个 allocator 互dealloc 安全）。有状态 allocator 要小心。
</details>

### Q4: 仿函数 vs 函数指针

```cpp
bool cmp_func(int a, int b) { return a < b; }
struct CmpFunctor {
    bool operator()(int a, int b) const { return a < b; }
};
std::sort(v.begin(), v.end(), cmp_func);    // A
std::sort(v.begin(), v.end(), CmpFunctor{}); // B
```
> A 和 B 在性能上有什么区别？

<details>
<summary>答案</summary>

**B 更快**。

- **A（函数指针）**：sort 内部通过指针间接调用 cmp_func，编译器通常无法内联
- **B（仿函数）**：`CmpFunctor` 类型已知，`operator()` 可内联，零调用开销

这就是 STL 强调函数对象而非函数指针的原因——**编译期类型已知 → 可内联**。

lambda 本质就是编译器生成的仿函数，性能等价 B。
</details>

## 参考与延伸

- 上一节：[本章概览](README.md)
- 下一节：[1.2 泛型编程](02-generic-programming.md)
