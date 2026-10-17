# 1.2 泛型编程与迭代器解耦

> 第 1 章 STL 概览 · 第 2 节 · 上一节：[1.1 六大组件](01-six-components.md) · 下一节：[1.3 SGI STL 源码组织](03-sgi-stl-source-organization.md)

## 为什么要学这个（先建立直觉）

在 C 里，"排序数组"和"排序链表"是完全不同的代码——数组用 `qsort`（随机访问），链表手写归并（顺序访问）。STL 用迭代器抽象层让同一个算法适配不同数据结构。

```c
/* C: 数组排序和链表排序完全不同 */
// 数组：qsort(arr, n, sizeof(int), cmp);
// 链表：手写 merge_sort，几十行代码

// 数组查找：bsearch（二分，O(log n)）
// 链表查找：手写循环（线性，O(n)）
// 两套完全不同的代码
```

```cpp
// C++ STL: find 对任何容器都适用
std::vector<int> v = {3, 1, 4};
auto it1 = std::find(v.begin(), v.end(), 4);  // vector

std::list<int> l = {3, 1, 4};
auto it2 = std::find(l.begin(), l.end(), 4);  // list

int arr[] = {3, 1, 4};
auto it3 = std::find(arr, arr + 3, 4);        // C 数组
// 同一个 find 模板，三种容器
```

**直觉**：迭代器是"泛型指针"——它抽象了"访问下一个元素"的操作，让算法不关心底层是连续数组还是链表节点。

## 这节讲什么

### 泛型编程的核心思想

泛型编程（Generic Programming）的关键洞察：**算法不应该绑定数据结构**。

STL 的解耦方式：
1. **容器**提供 `begin()`/`end()` 返回迭代器
2. **算法**只接收迭代器，通过迭代器读写元素
3. **迭代器**抽象了"遍历元素"的操作（`*`、`++`、`--`、`+n` 等）

### 迭代器分类决定算法能力

```cpp
// find 只需要 InputIterator（能 ++ 和 *）
template<typename InputIter, typename T>
InputIter find(InputIter first, InputIter last, const T& val) {
    while (first != last && *first != val) ++first;
    return first;
}
// 适用于：vector, list, deque, array, map, set, C 数组, istream...

// sort 需要 RandomAccessIterator（能 +n 和 []）
template<typename RandomAccessIter>
void sort(RandomAccessIter first, RandomAccessIter last);
// 适用于：vector, deque, array, C 数组
// 不适用于：list（只有 BidirectionalIterator）
```

### 迭代器是容器和算法的"接口契约"

```
容器承诺：         算法要求：
  begin() ──→ 迭代器 ──→ 满足某种 category
  end()                      ↓
                     算法按 category 选实现
```

迭代器分类（从弱到强）：
- **Input**：只读、单遍（`*`、`++`）
- **Output**：只写、单遍
- **Forward**：读写、多遍（`*`、`++`）
- **Bidirectional**：+ `--`
- **RandomAccess**：+ `[]`、`+n`、`-n`

**关键**：高 category 继承低 category（`RandomAccess` is-a `Bidirectional` is-a `Forward`...），算法可以按"最高可用 category"分派最优实现。

## 常见错误（新手踩坑）

### 错误 1：对 list 用 std::sort

```cpp
std::list<int> l = {3, 1, 4};
std::sort(l.begin(), l.end());  // 编译错误！list 迭代器不是随机访问
```

**修复**：`l.sort()`（成员函数，归并排序）。

### 错误 2：以为 find 是 O(1)

```cpp
std::set<int> s = {1, 2, 3, 4, 5};
auto it = std::find(s.begin(), s.end(), 3);  // O(n)！应该用 s.find(3) O(log n)
```

### 错误 3：混淆迭代器失效

```cpp
std::vector<int> v = {1, 2, 3};
auto it = v.begin();
v.push_back(4);  // 可能扩容 → it 失效！
// *it = 10;  // UB！
```

## 新手要点（和 C 的区别）

| 方面 | C | C++ STL |
|------|---|---------|
| 算法与数据结构 | 绑定（qsort 只排数组） | 解耦（sort 通配多种容器） |
| 遍历抽象 | 手写循环 + 下标/指针 | 迭代器统一接口 |
| 能力表达 | 无 | 迭代器分类（5 种 category） |
| 编译期分派 | 无 | traits + tag dispatch |

## HFT 关联

- **迭代器分类决定算法效率上限**：vector 随机访问 → sort 走内省排序 O(n log n)；list 双向 → 只能归并
- **copy 的 traits 特化**：随机访问 + trivially copyable → memmove，比逐元素快数倍
- **自定义容器要提供迭代器**：HFT 自建数据结构（如环形缓冲区）要实现迭代器接口才能用 STL 算法

## 代码自测

### Q1: find 的通用性

```cpp
std::vector<int> v = {1, 2, 3};
std::list<int> l = {1, 2, 3};
int arr[] = {1, 2, 3};
// 三个 find 调用实例化出几个模板版本？
auto it1 = std::find(v.begin(), v.end(), 2);
auto it2 = std::find(l.begin(), l.end(), 2);
auto it3 = std::find(arr, arr + 3, 2);
```

<details>
<summary>答案</summary>

**3 个模板实例化**（因为迭代器类型不同）：

- `find<int*, int>` — vector::iterator = int*, arr = int*（这两个可能是同一个实例化）
- `find<std::list<int>::iterator, int>` — list::iterator 是自定义类型

实际上如果 `v.begin()` 和 `arr` 都是 `int*`，编译器可能合并为 1 个实例化。所以总共 2 个实例化。

但 `find` 的**逻辑代码完全相同**——这就是泛型编程的价值：一份代码，多种类型。
</details>

### Q2: 迭代器分类与算法

```cpp
std::list<int> l = {3, 1, 4};
// 以下哪些能用？
// A: std::sort(l.begin(), l.end());
// B: std::find(l.begin(), l.end(), 3);
// C: std::reverse(l.begin(), l.end());
// D: std::lower_bound(l.begin(), l.end(), 3);
```

<details>
<summary>答案</summary>

- **A sort**：❌ 需要 RandomAccessIterator，list 只有 Bidirectional
- **B find**：✅ 只需 InputIterator
- **C reverse**：✅ 需要 BidirectionalIterator，list 满足
- **D lower_bound**：✅ 需要 ForwardIterator + 已排序，list 满足（但 O(n) 而非 O(log n)，因为 list 不能随机访问做二分）

注意：`lower_bound` 对 list 虽然能编译，但复杂度是 O(n) 而非 O(log n)——因为二分查找需要 `it + n` 随机跳转，list 只能逐步前进。
</details>

### Q3: 迭代器失效

```cpp
std::vector<int> v = {1, 2, 3};
auto it = v.begin() + 1;  // 指向 2
v.reserve(100);  // 可能重新分配内存
std::cout << *it;  // 安全吗？
```

<details>
<summary>答案</summary>

**不安全**。`reserve(100)` 如果当前 capacity < 100，会重新分配内存并搬迁元素，导致所有迭代器、指针、引用失效。

**修复**：reserve 后重新获取迭代器。

```cpp
v.reserve(100);
auto it = v.begin() + 1;  // 重新获取
std::cout << *it;  // 安全
```

**规则**：vector 扩容（reserve 超过当前 capacity、push_back 触发扩容）后，所有旧迭代器失效。
</details>

### Q4: 泛型算法的优势

```cpp
// 手写 C 版本：每种容器一套代码
int* find_int(int* begin, int* end, int val) {
    while (begin != end && *begin != val) ++begin;
    return begin;
}

// STL 版本：一套代码通配
template<typename Iter, typename T>
Iter find(Iter begin, Iter end, const T& val) {
    while (begin != end && *begin != val) ++begin;
    return begin;
}
```
> 泛型 find 比手写 find_int 多了什么能力？

<details>
<summary>答案</summary>

1. **类型通用**：能找 int、string、自定义类型——任何支持 `operator!=` 的类型
2. **容器通用**：能搜 vector、list、array、set、istream_iterator——任何提供迭代器的容器
3. **零开销抽象**：模板在编译期实例化，性能和手写专用版完全相同（甚至更好，因为编译器能跨内联优化）

这就是泛型编程的核心价值：**一次编写，处处复用，零运行时开销**。
</details>

## 参考与延伸

- 上一节：[1.1 六大组件](01-six-components.md)
- 下一节：[1.3 SGI STL 源码组织](03-sgi-stl-source-organization.md)
