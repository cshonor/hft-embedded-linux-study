# Item 32-33：`remove` + `erase` 惯用法（核心）

> 第 5 章 算法 · Item 32-33 · 下一节：[Item 34 binary_search/lower_bound](item34-binary-search-lower-bound.md)

## 为什么要学这个（先建立直觉）

C 程序员从数组中删除元素：

```c
// 删除所有等于 2 的元素
int j = 0;
for (int i = 0; i < n; ++i)
    if (arr[i] != 2) arr[j++] = arr[i];
n = j;  // 手动缩减长度
```

C++ 的 `std::remove` 做了同样的事——前移保留的元素——但它**不改变容器大小**：

```cpp
std::vector<int> v = {1, 2, 3, 2, 4};
auto new_end = std::remove(v.begin(), v.end(), 2);
// v = [1, 3, 4, ?, ?]  ← size 仍然是 5！
v.erase(new_end, v.end());  // ✅ 真正删除
// v = {1, 3, 4}
```

---

## 这节讲什么

`remove` 不删除元素——它是"覆盖式前移"，返回新逻辑末尾。`erase` 砍掉尾巴。指针容器删除有额外陷阱：被 `remove` 跳过的指针会泄漏。

---

## erase-remove 惯用法

```cpp
// 删除所有等于 val 的元素
v.erase(std::remove(v.begin(), v.end(), val), v.end());

// 删除所有满足条件的元素
v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());

// C++20 简化
std::erase(v, val);
std::erase_if(v, pred);
```

### remove 的本质

```cpp
std::vector<int> v = {1, 2, 3, 2, 4, 2, 5};
auto new_end = std::remove(v.begin(), v.end(), 2);

// remove 的执行过程（覆盖式前移）：
// [1, 2, 3, 2, 4, 2, 5]
//      ↓ 跳过 2，把 3 前移
// [1, 3, 3, 2, 4, 2, 5]
//         ↓ 跳过 2，把 4 前移
// [1, 3, 4, 2, 4, 2, 5]
//               ↓ 跳过 2，把 5 前移
// [1, 3, 4, 5, 4, 2, 5]
//  ^^^^^^^^^^^ ← new_end 指向这里（index 4）
//  保留部分      残留部分（未指定值）
```

### 指针容器陷阱

```cpp
std::vector<Widget*> v;
v.push_back(new Widget(1));
v.push_back(new Widget(2));  // 要删
v.push_back(new Widget(3));

// ❌ 直接 remove + erase → Widget(2) 泄漏
v.erase(std::remove(v.begin(), v.end(), target), v.end());

// ✅ 用智能指针
std::vector<std::unique_ptr<Widget>> v2;
v2.erase(std::remove_if(v2.begin(), v2.end(), pred), v2.end());
// 被移除的 unique_ptr 自动 delete
```

---

## 常见错误（新手踩坑）

### 错误 1：只调 remove 不调 erase

```cpp
std::vector<int> v = {1, 2, 3, 2, 4};
std::remove(v.begin(), v.end(), 2);
// v.size() 仍然是 5！末尾有垃圾值
```

**修正：** `v.erase(std::remove(...), v.end());`

### 错误 2：指针容器直接 remove

```cpp
std::vector<Widget*> v;
// ... 填充 ...
v.erase(std::remove(v.begin(), v.end(), target), v.end());
// target 指针被覆盖 → 泄漏
```

**修正：** 用 `vector<unique_ptr<Widget>>`。

### 错误 3：循环中 erase 迭代器

```cpp
for (auto it = v.begin(); it != v.end(); ++it) {
    if (*it == target) v.erase(it);  // ❌ it 失效
}
```

**修正：** `it = v.erase(it);` 或用 erase-remove。

---

## 新手要点（和 C 的区别）

| 维度 | C 删除 | C++ erase-remove | 为什么 |
|------|--------|-----------------|--------|
| 删除 | `j++` 前移 + `n=j` | `remove` 前移 + `erase` 砍尾 | 算法/容器分离 |
| 改变大小 | 手动 `n=j` | `erase` 改 size | 容器管理大小 |
| 条件删除 | 手写 if | `remove_if` + lambda | 声明式 |
| C++20 | N/A | `std::erase` / `std::erase_if` | 一步到位 |

**一句话：** C 的删除是手动前移 + 缩减长度。C++ 把"前移"（remove）和"缩减"（erase）分开——erase-remove 惯用法是 STL 核心惯用语，C++20 的 `std::erase` 让它更简洁。

---

## HFT 关联

- **erase-remove 清理无效档位**：撤单后批量清理订单簿用 `erase(remove_if(...))`，比循环 erase 更高效（一次移动 vs 多次）。
- **智能指针容器安全删除**：`vector<unique_ptr<Order>>` 删除元素时自动析构，无需手动 `delete`。
- **C++20 `std::erase_if`**：一行代码替代 erase-remove，更简洁且不易出错。

---

## 代码自测

### Q1: remove 不删除
```cpp
std::vector<int> v = {1, 2, 3, 2, 4};
auto new_end = std::remove(v.begin(), v.end(), 2);
std::cout << v.size();  // 输出？
```

<details>
<summary>答案</summary>

输出 **5**。`remove` 不改变容器大小，只前移保留元素。v 的物理内容 = `[1, 3, 4, ?, ?]`，size 仍然是 5。`new_end` 指向 index 3（逻辑末尾）。
</details>

### Q2: 正确删除
```cpp
std::vector<int> v = {1, 2, 3, 2, 4};
v.erase(std::remove(v.begin(), v.end(), 2), v.end());
// v 的内容？size？
```

<details>
<summary>答案</summary>

v = {1, 3, 4}，size = 3。`erase(new_end, v.end())` 砍掉了末尾的残留元素。
</details>

### Q3: remove_if + lambda
```cpp
std::vector<int> v = {1, 2, 3, 4, 5, 6};
v.erase(std::remove_if(v.begin(), v.end(),
    [](int x) { return x % 2 == 0; }), v.end());
// v 的内容？
```

<detailf>
<summary>答案</summary>

v = {1, 3, 5}。删除了所有偶数（2, 4, 6）。`remove_if` 用 lambda 判断条件，比手写循环更声明式。
</details>

### Q4: C++20 erase
```cpp
std::vector<int> v = {1, 2, 3, 2, 4, 2, 5};
std::erase(v, 2);
std::cout << v.size();
```

<detailf>
<summary>答案</summary>

输出 **4**。`std::erase(v, 2)` 删除所有等于 2 的元素，v = {1, 3, 4, 5}。

C++20 的 `std::erase` 是 erase-remove 惯用法的简化——一行代码，不需要手动调 `remove` + `erase`。
</details>

---

## 参考与延伸

- 下一节：[Item 34 binary_search/lower_bound](item34-binary-search-lower-bound.md)
- 回到：[第 5 章 算法](README.md)
