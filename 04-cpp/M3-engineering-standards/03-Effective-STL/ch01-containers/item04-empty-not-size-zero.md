# Item 4：用 empty() 而非 size()==0

> 第 1 章 容器 · Item 4 · 上一节：[Item 3 拷贝轻量且正确](item03-copy-lightweight-correct.md) · 下一节：[Item 5 优先区间成员函数](item05-prefer-range-members.md)

## 为什么要学这个（先建立直觉）

C 程序员判断数组是否为空：

```c
int arr[100];
int n = 0;
if (n == 0) { /* 空 */ }  // 直接比较长度
```

到了 C++ STL，你可能会写：

```cpp
if (v.size() == 0) { /* 空 */ }
```

这看起来没问题——但对 `std::list` 来说，`size()` 在 C++11 前可能是 O(n)（需要遍历整个链表计数）！而 `empty()` 对所有容器都是 O(1)。

---

## 这节讲什么

`empty()` 对所有容器都是 O(1)；`size()` 对 `list` 在 C++11 前可能是 O(n)。虽然 C++11 起 `list::size()` 强制 O(1)，但 `empty()` 表意更清晰且无歧义。

---

## 核心区别

```cpp
std::list<int> l;
// C++03: l.size() 可能是 O(n)，l.empty() 是 O(1)
// C++11+: 两者都是 O(1)，但 empty() 更清晰

if (l.empty())  { /* ✅ 清晰 + 保证 O(1) */ }
if (l.size()==0) { /* ⚠️ 可读性差 + 历史上可能 O(n) */ }
```

### 为什么 list::size() 曾经是 O(n)？

```cpp
// 一些 list 实现不维护 size 计数器（节省 splice() 的时间）
// splice() 把另一个 list 的节点移过来——如果不计数，O(1)
// 如果要计数，必须遍历源区间 → O(n)
// C++03 允许 splice 是 O(1) 而 size 是 O(n)
// C++11 要求 size() 是 O(1)，代价是 splice 变成 O(n)
```

---

## 常见错误（新手踩坑）

### 错误 1：在循环条件中用 size()==0

```cpp
while (v.size() > 0) {  // 可读性差
    v.pop_back();
}
// 虽然现代 C++ 中 vector::size() 是 O(1)，但为什么不写更清晰的？
```

**修正：** `while (!v.empty()) { v.pop_back(); }`

### 错误 2：用 size() 判断容器是否为空来避免操作

```cpp
if (v.size() == 0) return;  // 多余的比较
// 对空容器操作（如 v[0]）本身就是 UB，但 empty() 更直观
```

**修正：** `if (v.empty()) return;`

### 错误 3：混淆 size() 和 capacity()

```cpp
std::vector<int> v;
v.reserve(100);
if (v.size() == 100) { /* 不会进来——size 是 0，capacity 是 100 */ }
```

**修正：** 区分 `size()`（元素数）和 `capacity()`（容量）。`empty()` 判断的是 size。

---

## 新手要点（和 C 的区别）

| 维度 | C | C++ STL | 为什么 |
|------|---|---------|--------|
| 判空 | `n == 0` | `v.empty()` | 语义清晰 + 保证 O(1) |
| 长度 | `sizeof(arr)/sizeof(arr[0])` | `v.size()` | STL 维护计数器 |
| 容量 | 无 | `v.capacity()` | vector 预分配 |
| 统一性 | 数组操作统一 | 不同容器 size() 复杂度不同 | list 历史上 O(n) |

**一句话：** C 的数组长度是编译期常量，STL 的 size() 是运行时查询。用 `empty()` 判空——清晰、保证 O(1)、对所有容器统一。

---

## HFT 关联

- **热路径判空**：每 tick 检查订单簿是否为空用 `empty()`，保证 O(1) 无意外开销。
- **可读性即正确性**：`if (orderbook.empty())` 比 `if (orderbook.size() == 0)` 更直白，减少代码审查中的认知负担。

---

## 代码自测

### Q1: empty vs size
```cpp
std::list<int> l;
for (int i = 0; i < 1000000; ++i) l.push_back(i);

// C++03 实现（size 是 O(n)）
bool e1 = l.empty();       // A
bool e2 = (l.size() == 0); // B
```
> A 和 B 的复杂度分别是什么？（C++03 实现）

<details>
<summary>答案</summary>

- **A（empty()）**：O(1)。直接检查头指针是否指向尾哨兵。
- **B（size()==0）**：O(n)（C++03 的某些实现）。需要遍历整个链表计数。

C++11 起标准要求 `size()` 是 O(1)，但 `empty()` 仍然是更好的选择——清晰且无历史包袱。
</details>

### Q2: size vs capacity
```cpp
std::vector<int> v;
v.reserve(100);
std::cout << v.size() << ' ' << v.capacity();
```
> 输出什么？

<details>
<summary>答案</summary>

输出 `0 100`。`size()` = 0（没有元素），`capacity()` = 100（预分配了 100 个 int 的空间）。`reserve` 只改变 capacity，不改变 size。
</details>

### Q3: 可读性
```cpp
// 哪个写法更好？
if (v.size() != 0) { process(v); }      // A
if (!v.empty()) { process(v); }          // B
if (v.size() > 0) { process(v); }        // C
```

<details>
<summary>答案</summary>

**B 最好**。`!v.empty()` 直接表达"容器非空"的意图。`size() != 0` 和 `size() > 0` 都需要多一步心智转换（"size 不为 0 = 非空"）。

Scott Meyers 的建议：用 `empty()` 判空，用 `size()` 取元素数——别混用。
</details>

### Q4: list::splice 与 size
```cpp
std::list<int> a = {1, 2, 3};
std::list<int> b = {4, 5};
a.splice(a.end(), b);  // 把 b 的节点移到 a 末尾
// C++03 vs C++11: a.size() 的复杂度？
```

<details>
<summary>答案</summary>

- **C++03**：`a.size()` 可能是 O(n)（某些实现不在 splice 时更新计数器）。
- **C++11**：`a.size()` 是 O(1)，但 `splice` 变成 O(n)（需要遍历源区间计数）。

C++11 的设计权衡：`size()` O(1) 更常用，`splice` O(n) 可接受——因为 splice 通常用于已知大小的区间。
</details>

---

## 参考与延伸

- 上一节：[Item 3 拷贝轻量且正确](item03-copy-lightweight-correct.md)
- 下一节：[Item 5 优先区间成员函数](item05-prefer-range-members.md)
- 回到：[第 1 章 容器](README.md)
