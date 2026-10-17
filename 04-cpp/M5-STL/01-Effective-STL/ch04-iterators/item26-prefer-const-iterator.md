# Item 26：优先 `const_iterator`

> 第 4 章 迭代器 · Item 26 · 下一节：[Item 27-28 反向迭代器](item27-28-reverse-iterator-base.md)

## 为什么要学这个（先建立直觉）

C 程序员用 `const` 指针保护数据不被修改：

```c
const int* p = arr;  // 指向 const int，不能通过 p 修改数据
// *p = 42;  // 编译错误
```

C++ 的 `const_iterator` 同理——指向 `const` 元素，不能通过它修改数据：

```cpp
std::vector<int> v = {1, 2, 3};
auto it = v.cbegin();  // const_iterator
// *it = 42;  // 编译错误
```

---

## 这节讲什么

`const_iterator` 防止意外修改元素。C++11 起 `cbegin()`/`cend()` 让获取 `const_iterator` 更简单。配合 `auto` 使用，编译期杜绝误写只读数据。

---

## const_iterator 用法

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

// C++11: cbegin()/cend()
auto it = v.cbegin();  // vector<int>::const_iterator
// *it = 99;  // ❌ 编译错误

// C++03: begin() 返回 const_iterator（如果容器是 const）
const std::vector<int>& cv = v;
auto it2 = cv.begin();  // const_iterator

// STL 算法接受 const_iterator（C++14 起 insert/erase 也支持）
auto found = std::find(v.cbegin(), v.cend(), 3);  // 纯查找，不修改
```

---

## 常见错误（新手踩坑）

### 错误 1：用非 const 迭代器做只读遍历

```cpp
std::vector<int> v = {1, 2, 3};
for (auto it = v.begin(); it != v.end(); ++it)
    std::cout << *it;  // ⚠️ it 可以修改 *it，但没有 const 保护
```

**修正：** `for (auto it = v.cbegin(); it != v.cend(); ++it)`

### 错误 2：C++03 中获取 const_iterator 困难

```cpp
// C++03: 没有 cbegin()，要写完整类型
std::vector<int>::const_iterator it = v.begin();  // 隐式转换

// C++11: cbegin() + auto
auto it = v.cbegin();  // 简洁
```

**修正：** 用 C++11 `cbegin()`/`cend()` + `auto`。

### 错误 3：C++11 中 insert/erase 不接受 const_iterator

```cpp
std::vector<int> v = {1, 2, 3};
auto pos = v.cbegin() + 1;
// v.insert(pos, 99);  // C++11: 可能编译错误（某些实现）
// C++14 起：insert/erase 接受 const_iterator
```

**修正：** C++14 起标准保证 `insert`/`erase` 接受 `const_iterator`。

---

## 新手要点（和 C 的区别）

| 维度 | C `const` 指针 | C++ `const_iterator` | 为什么 |
|------|---------------|---------------------|--------|
| 获取 | `const T* p = arr` | `v.cbegin()` | C++11 简化 |
| 保护 | 编译期 | 编译期 | 相同 |
| 配合 auto | 无 | `auto it = v.cbegin()` | 类型推导 |

**一句话：** C 的 `const` 指针和 C++ 的 `const_iterator` 本质相同——编译期防止误写。C++11 的 `cbegin()`/`cend()` + `auto` 让获取 `const_iterator` 更简洁。

---

## HFT 关联

- **`const_iterator` 防误改**：行情快照遍历用 `cbegin()`/`cend()`，编译期杜绝误写只读数据。
- **`find` 用 `const_iterator`**：纯查找不修改，用 `std::find(v.cbegin(), v.cend(), val)` 表意更清晰。

---

## 代码自测

### Q1: const_iterator 保护
```cpp
std::vector<int> v = {1, 2, 3};
auto it = v.cbegin();
*it = 99;  // 编译通过吗？
```

<details>
<summary>答案</summary>

**编译错误**。`cbegin()` 返回 `const_iterator`，`*it` 的类型是 `const int&`，不能赋值。
</details>

### Q2: cbegin vs begin
```cpp
std::vector<int> v = {1, 2, 3};
auto it1 = v.begin();    // A
auto it2 = v.cbegin();   // B
```
> it1 和 it2 的类型分别是什么？

<details>
<summary>答案</summary>

- **it1**：`std::vector<int>::iterator`（可修改元素）
- **it2**：`std::vector<int>::const_iterator`（不可修改元素）

纯查找/遍历时用 `cbegin()`/`cend()`，需要修改时用 `begin()`/`end()`。
</details>

---

## 参考与延伸

- 下一节：[Item 27-28 反向迭代器](item27-28-reverse-iterator-base.md)
- 回到：[第 4 章 迭代器](README.md)
