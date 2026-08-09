# Item 9：删除元素的正确方式

> 第 1 章 容器 · Item 9 · 上一节：[Item 8 不存 auto_ptr](item08-no-auto-ptr.md) · 下一节：[Item 10-11 分配器](item10-11-allocators.md)

## 为什么要学这个（先建立直觉）

C 程序员从数组中删除元素：

```c
// 删除第 i 个元素
memmove(&arr[i], &arr[i+1], (n - i - 1) * sizeof(int));
n--;  // 手动减长度
```

C++ STL 的 `remove` 算法**不删除元素**——它只是前移保留的元素，返回新逻辑末尾。真正删除要配合 `erase`：

```cpp
std::vector<int> v = {1, 2, 3, 2, 4};
std::remove(v.begin(), v.end(), 2);  // v 仍然是 5 个元素！
v.erase(std::remove(v.begin(), v.end(), 2), v.end());  // ✅ 真正删除
```

---

## 这节讲什么

STL 的 `remove`/`remove_if` 不改变容器大小——它们是"搬运"而非"删除"。真正删除要用 erase-remove 惯用法。指针容器删除有额外陷阱：被 `remove` 跳过的指针会泄漏。

---

## erase-remove 惯用法

```cpp
std::vector<int> v = {1, 2, 3, 2, 4, 2, 5};

// remove 做了什么？
auto new_end = std::remove(v.begin(), v.end(), 2);
// v 的物理内容：[1, 3, 4, 5, ?, ?, ?]  ← size 仍然是 7
// new_end 指向第一个 ?（逻辑末尾）

// erase 砍掉尾巴
v.erase(new_end, v.end());
// v = {1, 3, 4, 5}  ← size 变成 4

// C++20 简化
std::erase(v, 2);  // 一行搞定
```

### remove_if + erase

```cpp
// 删除所有偶数
v.erase(std::remove_if(v.begin(), v.end(),
    [](int x) { return x % 2 == 0; }), v.end());

// C++20
std::erase_if(v, [](int x) { return x % 2 == 0; });
```

### 指针容器的删除陷阱

```cpp
std::vector<Widget*> v;
v.push_back(new Widget(1));
v.push_back(new Widget(2));  // 要删这个
v.push_back(new Widget(3));

// ❌ 直接 remove + erase → Widget(2) 泄漏！
// remove 把 Widget(3) 的指针前移覆盖了 Widget(2) 的位置
// Widget(2) 的指针丢失了 → 无法 delete → 泄漏

// ✅ 先 delete 要删的，再 remove
std::for_each(v.begin(), v.end(),
    [](Widget* p) { if (should_delete(p)) delete p; });
v.erase(std::remove_if(v.begin(), v.end(),
    [](Widget* p) { return p == nullptr; }), v.end());

// ✅✅ 用智能指针，erase-remove 自动析构
std::vector<std::unique_ptr<Widget>> v2;
v2.erase(std::remove_if(v2.begin(), v2.end(), should_delete),
         v2.end());  // 被移除的 unique_ptr 自动 delete
```

---

## 常见错误（新手踩坑）

### 错误 1：只调 remove 不调 erase

```cpp
std::vector<int> v = {1, 2, 3, 2, 4};
std::remove(v.begin(), v.end(), 2);  // v.size() 仍然是 5！
// v = {1, 3, 4, ?, ?}（末尾是垃圾值）
```

**修正：** `v.erase(std::remove(v.begin(), v.end(), 2), v.end());`

### 错误 2：指针容器直接 remove + erase

```cpp
std::vector<Widget*> v;
// ... 填充 ...
v.erase(std::remove(v.begin(), v.end(), target), v.end());
// target 指针被覆盖丢失 → Widget 对象泄漏
```

**修正：** 用 `vector<unique_ptr<Widget>>`，或先 `for_each + delete` 再 `remove`。

### 错误 3：在循环中 erase 迭代器

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
for (auto it = v.begin(); it != v.end(); ++it) {
    if (*it % 2 == 0) v.erase(it);  // ❌ it 失效！
}
```

**修正：** `it = v.erase(it);`（erase 返回下一个有效迭代器），或用 erase-remove。

---

## 新手要点（和 C 的区别）

| 维度 | C | C++ STL | 为什么 |
|------|---|---------|--------|
| 删除 | `memmove` + `n--` | `erase(remove(...))` | 算法+容器分离 |
| 删除语义 | 直接改数组 | remove 搬运 + erase 砍尾 | 设计权衡 |
| 条件删除 | 手写循环 | `remove_if` + lambda | 声明式 |
| 指针删除 | 手动 free | 智能指针自动析构 | RAII |

**一句话：** C 的删除是 `memmove` + 手动减长度，一步到位。STL 把"搬运"（remove）和"删除"（erase）分开——erase-remove 惯用法是必须掌握的 STL 惯用语。

---

## HFT 关联

- **erase-remove 清理无效档位**：撤单后批量清理订单簿用 `erase(remove_if(...))`，比循环 erase 更高效（一次移动 vs 多次）。
- **智能指针容器安全删除**：`vector<unique_ptr<Order>>` 删除元素时自动析构，无需手动 `delete`，异常安全。
- **C++20 `std::erase`**：一行代码替代 erase-remove 惯用法，更简洁且不易出错。

---

## 代码自测

### Q1: remove 不删除
```cpp
std::vector<int> v = {1, 2, 3, 2, 4};
auto new_end = std::remove(v.begin(), v.end(), 2);
std::cout << v.size();  // 输出多少？
```

<details>
<summary>答案</summary>

输出 **5**。`remove` 不改变容器大小，只把不等于 2 的元素前移。v 的物理内容变为 `[1, 3, 4, ?, ?]`（`?` 是残留值），size 仍然是 5。`new_end` 指向第 4 个位置（逻辑末尾）。

要真正删除：`v.erase(new_end, v.end());`
</details>

### Q2: 指针容器泄漏
```cpp
std::vector<Widget*> v;
v.push_back(new Widget(1));
v.push_back(new Widget(2));
v.push_back(new Widget(3));

// 删除 Widget(2)
v.erase(std::remove(v.begin(), v.end(), v[1]), v.end());
// 有什么问题？
```

<details>
<summary>答案</summary>

**Widget(2) 泄漏**。`remove` 把后面的指针（Widget(3)）前移覆盖了 Widget(2) 的位置，Widget(2) 的指针丢失了，无法 `delete`。

**修正：**
1. 先 `delete v[1]` 再 `remove`：
```cpp
delete v[1]; v[1] = nullptr;
v.erase(std::remove(v.begin(), v.end(), nullptr), v.end());
```
2. 或用 `vector<unique_ptr<Widget>>`。
</details>

### Q3: erase 迭代器
```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
for (auto it = v.begin(); it != v.end(); ) {
    if (*it % 2 == 0) {
        it = v.erase(it);  // erase 返回下一个迭代器
    } else {
        ++it;
    }
}
// v 的内容？
```

<details>
<summary>答案</summary>

v = {1, 3, 5}。删掉了所有偶数（2 和 4）。

关键：`erase` 返回被删元素之后位置的迭代器，不能用 `++it`（it 已失效）。用 erase-remove 更简洁：
```cpp
v.erase(std::remove_if(v.begin(), v.end(),
    [](int x){ return x%2==0; }), v.end());
```
</details>

### Q4: C++20 erase
```cpp
std::vector<int> v = {1, 2, 3, 2, 4, 2, 5};
std::erase(v, 2);
// vs
std::erase_if(v, [](int x){ return x > 3; });
```
> 两次操作后 v 的内容分别是什么？

<details>
<summary>答案</summary>

- `std::erase(v, 2)` 后：v = {1, 3, 4, 5}（删除所有等于 2 的元素）
- 接着 `std::erase_if(v, [](int x){ return x > 3; })` 后：v = {1, 3}（删除所有 >3 的元素）

C++20 的 `std::erase`/`std::erase_if` 是 erase-remove 惯用法的简化版，一行代码完成删除。
</details>

---

## 参考与延伸

- 上一节：[Item 8 不存 auto_ptr](item08-no-auto-ptr.md)
- 下一节：[Item 10-11 分配器](item10-11-allocators.md)
- 回到：[第 1 章 容器](README.md)
