# 3.1 五种迭代器分类

> 第 3 章 迭代器与 traits · 第 1 节 · 上一节：[本章概览](README.md) · 下一节：[3.2 关联类型](02-associated-types.md)

## 为什么要学这个（先建立直觉）

在 C 里，指针的能力是"全有或全无"——要么随机访问（数组指针），要么不能用。STL 把迭代器能力分成 5 个层级，让算法按"最低必要能力"编写，实现最大通用性。

```c
/* C: 指针能力没有分级 */
int arr[] = {1, 2, 3};
// arr+2: 随机访问 OK
// qsort(arr, 3, sizeof(int), cmp): 需要随机访问

struct Node { int val; struct Node* next; };
// 链表指针只能 p = p->next，不能 p+2
// 不能用 qsort，只能手写归并
// 但 C 没有表达"这个指针只能前进"的机制
```

```cpp
// C++ STL: 5 种迭代器分类，算法按最低能力要求编写
// find 只需 InputIterator（能 ++ 和 *）→ vector/list/array 都能用
// sort 需 RandomAccessIterator（能 +n）→ 只有 vector/deque/array 能用
// list 只有 BidirectionalIterator → 不能用 sort，用成员 sort()
```

**直觉**：迭代器分类是"能力等级"——高等级继承低等级的所有能力。算法声明"我需要什么等级的迭代器"，低于这个等级的容器就不能用。

## 这节讲什么

### 五种分类与能力

| 分类 | 能力 | C 对应 | 典型容器 |
|------|------|--------|----------|
| Input | `*`(读)、`++`、`==` | 只前进的文件指针 | istream_iterator |
| Output | `*`(写)、`++` | 只写的输出流 | back_inserter, ostream_iterator |
| Forward | `*`(读写)、`++`、多遍 | 单链表指针 | forward_list |
| Bidirectional | + `--` | 双链表指针 | list, set, map |
| RandomAccess | + `[]`、`+n`、`-n`、`<` | 数组指针 | vector, deque, array |

### 标签继承层级

```cpp
struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag : input_iterator_tag {};           // 继承 input
struct bidirectional_iterator_tag : forward_iterator_tag {};     // 继承 forward
struct random_access_iterator_tag : bidirectional_iterator_tag {}; // 继承 bidirectional
```

继承让算法能用"最具体匹配"分派——传 `random_access_iterator_tag` 时，既能匹配 `random_access` 重载，也能匹配 `input` 重载（但编译器选最具体的）。

### 算法对迭代器的要求

```cpp
// find: 只需 InputIterator（能 ++ 和 *）
template<typename InputIter, typename T>
InputIter find(InputIter first, InputIter last, const T& val);

// reverse: 需 BidirectionalIterator（能 ++ 和 --）
template<typename BidirIter>
void reverse(BidirIter first, BidirIter last);

// sort: 需 RandomAccessIterator（能 +n 和 []）
template<typename RAIter>
void sort(RAIter first, RAIter last);

// list::iterator 是 Bidirectional → 不能用 sort，能用 reverse
// vector::iterator 是 RandomAccess → 都能用
```

## 常见错误（新手踩坑）

### 错误 1：对 list 用 std::sort

```cpp
std::list<int> l = {3, 1, 4};
std::sort(l.begin(), l.end());  // 编译错误：list 迭代器不是 RandomAccess
```

### 错误 2：混淆 Input 和 Forward

```cpp
// InputIterator 是单遍的——遍历后迭代器可能失效
std::istream_iterator<int> it(std::cin), end;
int first = *it; ++it;
// 不能假设 *it 还能再次读到 first（流是单遍的）

// ForwardIterator 是多遍的——可以多次遍历
std::forward_list<int> fl = {1, 2, 3};
auto it2 = fl.begin();
int a = *it2; ++it2;
int b = *it2;
// 可以回到 fl.begin() 再读一遍
```

### 错误 3：以为 deque 和 vector 迭代器一样快

```cpp
// deque::iterator 虽然是 RandomAccess，但跨段时需跳转到不同缓冲区
// 比 vector::iterator（原生指针）慢——两步间接 vs 一步
```

## 新手要点（和 C 的区别）

| 方面 | C | C++ STL |
|------|---|---------|
| 指针能力 | 无分级 | 5 种 category |
| 算法通用性 | 低（每种数据结构一套） | 高（按 category 匹配） |
| 编译期检查 | 无 | 迭代器不匹配 → 编译错误 |
| 能力表达 | 注释 | 标签继承 + traits |

## HFT 关联

- **vector 随机访问 → sort 走内省排序**：迭代器分类决定算法选型，vector 比 list 多一个"能用 std::sort"的性能优势
- **copy 对 RandomAccess + POD 走 memmove**：迭代器分类 + traits 联合分派，零开销优化
- **自定义容器要选择正确的迭代器分类**：HFT 环形缓冲区如果支持 `+n` 就声明 RandomAccess，让 STL 算法选最优路径

## 代码自测

### Q1: 分类匹配

```cpp
std::vector<int> v;
std::list<int> l;
std::forward_list<int> fl;

std::find(v.begin(), v.end(), 42);    // A
std::find(l.begin(), l.end(), 42);    // B
std::find(fl.begin(), fl.end(), 42);  // C
std::sort(v.begin(), v.end());        // D
std::sort(l.begin(), l.end());        // E
std::reverse(l.begin(), l.end());     // F
std::reverse(fl.begin(), fl.end());   // G
```
> 哪些能编译？

<details>
<summary>答案</summary>

- **A, B, C（find）**：✅ find 需 InputIterator，三者都满足
- **D（sort vector）**：✅ sort 需 RandomAccessIterator，vector 满足
- **E（sort list）**：❌ list 只有 Bidirectional，不满足 RandomAccess
- **F（reverse list）**：✅ reverse 需 BidirectionalIterator，list 满足
- **G（reverse forward_list）**：❌ forward_list 只有 Forward，不满足 Bidirectional

forward_list 只有 Forward 迭代器——不能 `--`，所以不能用 reverse/sort 等需要双向或随机访问的算法。
</details>

### Q2: 标签继承

```cpp
void func(std::input_iterator_tag) { std::cout << "input"; }
void func(std::random_access_iterator_tag) { std::cout << "random"; }

std::vector<int>::iterator it;
func(std::iterator_traits<decltype(it)>::iterator_category{});
```
> 输出是什么？为什么？

<details>
<summary>答案</summary>

输出 **random**。

`vector<int>::iterator` 的 category 是 `random_access_iterator_tag`。虽然 `random_access_iterator_tag` 继承 `input_iterator_tag`，但重载决议选**最具体**的匹配 → `random_access_iterator_tag` 版本。

如果只定义了 `input_iterator_tag` 版本，传 `random_access_iterator_tag` 也能匹配（通过继承），输出 "input"。
</details>

### Q3: istream_iterator 分类

```cpp
std::istream_iterator<int> it(std::cin), end;
std::vector<int> v(it, end);  // 从 cin 读入 vector
// v 的构造完后，it 还能再用吗？
```

<details>
<summary>答案</summary>

**不能保证**。`istream_iterator` 是 InputIterator——单遍遍历。读过的数据不会回来（流是消费型的）。

`vector` 的构造函数在构造过程中遍历了 `[it, end)`，消耗了流中的数据。构造完后 `it` 可能已失效（到达流末尾）。

**对比 ForwardIterator**：`forward_list` 的迭代器可以多次遍历同一范围，数据不会消失。

**教训**：InputIterator 是"一次性"的——用过就没了。
</details>

### Q4: 自定义迭代器分类

```cpp
class RingBufferIter {
    // 我的环形缓冲区迭代器支持 *, ++, --, +n, -n, [], <, ==
    // 应该声明为哪种 category？
};
```

<details>
<summary>答案</summary>

声明为 **RandomAccessIterator**，因为支持所有随机访问操作。

```cpp
class RingBufferIter {
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using pointer = int*;
    using reference = int&;

    reference operator*() const;
    RingBufferIter& operator++();
    RingBufferIter& operator--();
    RingBufferIter& operator+=(difference_type n);
    difference_type operator-(const RingBufferIter& other) const;
    reference operator[](difference_type n) const;
    bool operator==(const RingBufferIter& other) const;
    bool operator<(const RingBufferIter& other) const;
};
```

声明为 RandomAccess 后，`std::sort` 等算法就能用于你的环形缓冲区。

**HFT**：自定义容器的迭代器分类要诚实——声明的能力必须全部实现，否则算法行为未定义。
</details>

## 参考与延伸

- 上一节：[本章概览](README.md)
- 下一节：[3.2 关联类型](02-associated-types.md)
