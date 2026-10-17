# Item 5：优先区间成员函数

> 第 1 章 容器 · Item 5 · 上一节：[Item 4 empty() 而非 size()==0](item04-empty-not-size-zero.md) · 下一节：[Item 6 警惕最烦人解析](item06-most-vexing-parse.md)

## 为什么要学这个（先建立直觉）

C 程序员批量拷贝数据用 `memcpy`/`memmove`——一次调用处理整段：

```c
int src[100], dst[100];
memcpy(dst, src, 100 * sizeof(int));  // 一次操作，O(n)
```

但用 C++ STL 时，新手常写循环逐个插入：

```cpp
std::vector<int> v;
for (auto it = src.begin(); it != src.end(); ++it)
    v.push_back(*it);  // 可能多次扩容！
```

STL 提供了**区间成员函数**——一次调用处理整段区间，更高效且更清晰：

```cpp
v.insert(v.end(), src.begin(), src.end());  // 区间版：一次操作
```

---

## 这节讲什么

区间成员函数（`assign`/`insert`/`erase` 的区间版本）比单元素循环高效——区间版一次知道范围，可批量预留/移动；循环版每次单插，多次扩容 + 移动。

---

## 区间 vs 单元素

```cpp
std::vector<int> src = {1, 2, 3, 4, 5};
std::vector<int> v;

// ❌ 循环 push_back——可能多次扩容
for (auto it = src.begin(); it != src.end(); ++it)
    v.push_back(*it);

// ✅ 区间 insert——一次操作，实现可预计算距离并 reserve
v.insert(v.end(), src.begin(), src.end());

// ✅ 区间 assign——替换全部内容
v.assign(src.begin(), src.end());

// ✅ 区间 erase——一次删除一段
v.erase(v.begin(), v.begin() + 3);  // 删前 3 个
```

### 为什么区间版更快？

```cpp
// 区间 insert 知道 [first, last) 的距离（对随机访问迭代器）
// → 可以一次 reserve(distance) + memcpy/memmove
// 循环 push_back 不知道总量 → 多次扩容（2x 增长策略）
```

### 构造也支持区间

```cpp
std::vector<int> v(src.begin(), src.end());  // 区间构造
// 比先默认构造再循环 push_back 高效
```

---

## 常见错误（新手踩坑）

### 错误 1：循环 push_back 导致多次扩容

```cpp
std::vector<int> v;
for (int i = 0; i < 1000; ++i)
    v.push_back(data[i]);  // ~10 次扩容
```

**修正：** `v.insert(v.end(), data, data + 1000);` 或 `v.assign(data, data + 1000);`

### 错误 2：循环 erase 导致多次元素移动

```cpp
// 删除前 5 个元素
for (int i = 0; i < 5; ++i)
    v.erase(v.begin());  // 每次 erase 都移动后面所有元素 → O(5n)
```

**修正：** `v.erase(v.begin(), v.begin() + 5);` 一次移动 → O(n)

### 错误 3：用 copy + back_inserter 而非区间 insert

```cpp
std::copy(src.begin(), src.end(), std::back_inserter(v));
// 虽然能工作，但不如区间 insert 高效——copy 不知道目标容量
```

**修正：** `v.insert(v.end(), src.begin(), src.end());`

---

## 新手要点（和 C 的区别）

| 维度 | C | C++ STL | 为什么 |
|------|---|---------|--------|
| 批量拷贝 | `memcpy` | `assign(first, last)` | 类型安全 |
| 批量插入 | 循环赋值 | `insert(pos, first, last)` | 可预分配 |
| 批量删除 | `memmove` | `erase(first, last)` | 调用析构 |
| 区间构造 | 无 | `Container(first, last)` | 一步到位 |

**一句话：** C 的 `memcpy`/`memmove` 是批量操作的鼻祖。STL 的区间成员函数是类型安全的 `memcpy`——优先用区间版，省去循环和多次扩容。

---

## HFT 关联

- **区间 insert 批量拷贝 tick**：从行情缓冲批量拷贝到处理队列用 `v.insert(v.end(), buf, buf + n)`，一次操作，可能走 `memmove` 特化。
- **区间 erase 批量清理**：撤单后批量删除无效档位用 `v.erase(first, last)`，一次移动而非多次。
- **`reserve` + 区间 insert = 零扩容**：先 `reserve(n)` 再区间 `insert`，热路径零扩容零拷贝尖峰。

---

## 代码自测

### Q1: 区间 vs 循环性能
```cpp
std::vector<int> src = /* 10000 个元素 */;

// A
std::vector<int> v1;
for (auto x : src) v1.push_back(x);

// B
std::vector<int> v2;
v2.insert(v2.end(), src.begin(), src.end());
```
> A 和 B 各发生多少次内存分配？

<details>
<summary>答案</summary>

- **A**：约 14 次分配（2x 增长：1→2→4→...→16384，log₂(10000)≈14）。每次分配 = 新内存 + 拷贝旧元素 + 释放旧内存。
- **B**：1 次分配（区间 insert 对随机访问迭代器可计算距离，一次 reserve）。
</details>

### Q2: 区间 erase
```cpp
std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// 删除前 3 个
// A
for (int i = 0; i < 3; ++i) v.erase(v.begin());

// B
v.erase(v.begin(), v.begin() + 3);
```
> A 和 B 的复杂度分别是什么？

<details>
<summary>答案</summary>

- **A**：O(3n) ≈ O(n)。每次 `erase(begin())` 都要把后面所有元素前移一位。3 次 erase = 3 次移动。
- **B**：O(n)。一次移动，把第 4 个到末尾的元素整体前移 3 位。

B 更高效——一次 memmove vs 三次 memmove。
</details>

### Q3: assign 的语义
```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
std::vector<int> src = {10, 20, 30};

v.assign(src.begin(), src.end());
// v 的内容？size？capacity？
```

<details>
<summary>答案</summary>

- v = {10, 20, 30}
- size = 3（assign 替换全部内容，旧元素被析构）
- capacity ≥ 3（可能保留旧 capacity，取决于实现）

`assign` = "清空 + 区间插入"，比 `v.clear(); v.insert(...)` 更高效（一次操作）。
</details>

### Q4: 区间构造
```cpp
int arr[] = {1, 2, 3, 4, 5};

// A
std::vector<int> v1;
v1.resize(5);
for (int i = 0; i < 5; ++i) v1[i] = arr[i];

// B
std::vector<int> v2(arr, arr + 5);
```
> B 比 A 好在哪里？

<details>
<summary>答案</summary>

1. **更高效**：B 一次分配 + 一次拷贝；A 先默认构造 5 个元素（值初始化为 0），再逐个赋值。
2. **更简洁**：一行代码 vs 三行。
3. **更安全**：B 不会忘记 resize 导致越界。

区间构造是 C 中 `memcpy(dst, src, n * sizeof(int))` 的类型安全替代。
</details>

---

## 参考与延伸

- 上一节：[Item 4 empty() 而非 size()==0](item04-empty-not-size-zero.md)
- 下一节：[Item 6 警惕最烦人解析](item06-most-vexing-parse.md)
- 回到：[第 1 章 容器](README.md)
