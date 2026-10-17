# 第 4 章 迭代器

**Iterators** — Items 26–31

## 本章讲什么

迭代器是容器与算法之间的桥梁。本章讲 `const_iterator` 的正确使用、反向迭代器与正向的转换、流迭代器、以及迭代器分类（输入/输出/前向/双向/随机访问）如何决定可用算法。

---

## 各 Item 要点

### Item 26：优先 `const_iterator`

`const_iterator` 防止意外修改元素。C++11 起 `cbegin()`/`cend()` 让获取 `const_iterator` 更简单，且 STL 算法（`find`/`insert`）在 C++14 起支持 `const_iterator`。配合 `auto`：`auto it = v.cbegin();`。

### Item 27–28：反向迭代器与 `base()`

`rbegin()`/`rend()` 给出反向迭代器。要把反向迭代器转回正向位置用 `base()`——但 `base()` 指向的位置比原反向迭代器**偏后一个元素**（反向迭代的语义）。删除元素时这个偏移是经典 off-by-one 来源：

```cpp
auto rit = std::find(v.rbegin(), v.rend(), target);
v.erase(rit.base() - 1);  // 删除 target，注意 -1
```

### Item 29：`istream_iterator` / `ostream_iterator`

流迭代器让流当容器用：`std::copy(istream_iterator<int>(cin), {}, back_inserter(v));` 从 cin 读入 vector。简洁但每次 `++` 都解析一次流，性能不如批量 `read`。

### Item 30：`iostreambuf_iterator` vs `istream_iterator`

`istreambuf_iterator` 直接读字节流（不跳空白、不格式化），比 `istream_iterator`（跳空白、格式化）快。读原始二进制用前者。

### Item 31：了解迭代器分类与可用算法

| 分类 | 能力 | 典型容器 |
|------|------|----------|
| 输入迭代器 | 只读、单遍、`++` | `istream_iterator` |
| 输出迭代器 | 只写、单遍、`++` | `back_inserter` |
| 前向迭代器 | 读写、多遍、`++` | `forward_list`、`unordered_*` |
| 双向迭代器 | + `--` | `list`、`map`/`set` |
| 随机访问迭代器 | + `[]`、`+`/`-` | `vector`、`deque`、`array` |

算法对迭代器分类有要求：`sort` 要随机访问（`list` 不能用 `sort`，要用成员 `sort()`）；`reverse` 要双向。选错编译失败或运行错误。

---

## HFT 关联

- **`const_iterator` 防误改**：行情快照遍历用 `cbegin()`/`cend()`，编译期杜绝误写只读数据。
- **随机访问迭代器换 `sort`**：`vector` 的随机访问迭代器让 `std::sort` 高效（内省排序）；`list` 只能用成员 `sort()`（归并），性能差——这也是 HFT 选 `vector` 的原因之一。
- **流迭代器慎用**：`istream_iterator` 解析开销大，HFT 行情解析用 `string_view` + 手写解析器，不用流迭代器。

---

## 自测题

1. `rbegin()`/`rend()` 与 `begin()`/`end()` 的方向关系是什么？`base()` 有什么偏移？
2. 五种迭代器分类各自的能力是什么？`std::sort` 要求哪种？
3. `istream_iterator` 和 `istreambuf_iterator` 的区别？哪个更快？
4. 为什么 `list` 不能用 `std::sort`？该用什么？
5. `cbegin()`/`cend()` 配合 `auto` 如何防止意外修改？

## 代码自测

### Q1: 迭代器失效
```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

auto it = v.begin() + 2;  // 指向 3
v.push_back(6);            // 可能扩容

std::cout << *it;  // 安全吗？
```
> `push_back` 后 `it` 还有效吗？哪些操作会让 vector 迭代器失效？

<details>
<summary>答案与复习指引</summary>

**不安全**。`push_back` 如果触发扩容（capacity 不够），所有迭代器、指针、引用**全部失效**（旧内存被释放）。即使不扩容，`push_back` 也只保证 `end()` 失效，但实现可能不保证。

**vector 迭代器失效规则**：

| 操作 | 失效范围 |
|------|---------|
| `push_back` | 扩容→全部失效；不扩容→`end()` 失效 |
| `insert(pos, val)` | pos 之后全部失效（扩容则全部） |
| `erase(pos)` | pos 及之后失效 |
| `reserve/clear` | 全部失效 |

**HFT 实践**：热路径不动态增删 vector，启动时 reserve 固定大小，避免迭代器失效。

**复习：** → [迭代器失效规则](./README.md)
</details>

### Q2: 迭代器分类
```cpp
std::vector<int> v = {3, 1, 4};
std::list<int> l = {3, 1, 4};

std::sort(v.begin(), v.end());     // A: OK
// std::sort(l.begin(), l.end());  // B: 编译失败
l.sort();                           // C: OK
```
> 为什么 B 编译失败？vector 和 list 的迭代器分别属于哪个分类？

<details>
<summary>答案与复习指引</summary>

**B 编译失败**：`std::sort` 要求**随机访问迭代器**（RandomAccessIterator）。`vector::iterator` 是随机访问（支持 `it + n`），`list::iterator` 只是**双向迭代器**（BidirectionalIterator，只支持 `++`/`--`，不支持 `+n`）。

**五大迭代器分类**（从弱到强）：
1. InputIterator — 只读，单遍（`istream_iterator`）
2. OutputIterator — 只写，单遍（`back_inserter`）
3. ForwardIterator — 读写，多遍（`forward_list`）
4. BidirectionalIterator — 可前后（`list`、`map`）
5. RandomAccessIterator — 随机访问（`vector`、`deque`、原生指针）

`list::sort()` 是成员函数，利用链表特性（指针重连）实现归并排序，不需要随机访问。

**复习：** → [迭代器分类](./README.md)
</details>
