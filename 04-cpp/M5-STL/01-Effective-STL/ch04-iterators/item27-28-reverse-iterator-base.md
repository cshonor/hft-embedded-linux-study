# Item 27-28：反向迭代器与 `base()`

> 第 4 章 迭代器 · Item 27-28 · 上一节：[Item 26 const_iterator](item26-prefer-const-iterator.md) · 下一节：[Item 29 流迭代器](item29-stream-iterators.md)

## 为什么要学这个（先建立直觉）

C 程序员反向遍历数组：

```c
for (int i = n - 1; i >= 0; --i) {
    process(arr[i]);
}
```

C++ 的反向迭代器更优雅：

```cpp
for (auto it = v.rbegin(); it != v.rend(); ++it)
    process(*it);  // 从尾到头遍历
```

但要把反向迭代器转回正向位置用 `base()`——`base()` 指向的位置比原反向迭代器**偏后一个元素**，这是经典的 off-by-one 来源。

---

## 这节讲什么

`rbegin()`/`rend()` 给出反向迭代器。`base()` 转换为正向迭代器，但偏移一个位置。删除元素时这个偏移是经典 off-by-one 来源。

---

## base() 的偏移

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
//                  位置: 0  1  2  3  4
// 正向: begin()→1  end()→(past 5)
// 反向: rbegin()→5  rend()→(before 1)

auto rit = std::find(v.rbegin(), v.rend(), 3);
// rit 指向 3（反向遍历找到的第一个 3）
// rit.base() 指向 4（3 的下一个元素！）

// 删除 3：
v.erase(rit.base() - 1);  // ✅ 减 1 才指向 3
// v.erase(rit.base());    // ❌ 这会删除 4！
```

图示：
```
正向:  [1]  [2]  [3]  [4]  [5]
                ^rit  ^base
            （rit 指向 3，base 指向 4）
```

---

## 常见错误（新手踩坑）

### 错误 1：erase 用 base() 不减 1

```cpp
auto rit = std::find(v.rbegin(), v.rend(), target);
v.erase(rit.base());  // ❌ 删了 target 后面一个元素
```

**修正：** `v.erase(rit.base() - 1);` 或 `v.erase(std::next(rit).base());`

### 错误 2：在正向插入时用 base()

```cpp
auto rit = std::find(v.rbegin(), v.rend(), 3);
v.insert(rit.base(), 99);  // 在 3 后面插入 99
// 如果想在 3 前面插入：v.insert(rit.base() - 1, 99);
```

**注意：** `insert(pos, val)` 在 `pos` 前插入。`base()` 指向 3 的后面，所以 `insert(base(), 99)` 在 3 和 4 之间插入。

### 错误 3：混淆 rbegin/rend 的方向

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
// rbegin() 指向 5（最后一个元素）
// rend() 指向 1 之前（过去开始）
// ++rbegin() 指向 4（向前一个 = 反向的"下一个"）
```

**注意：** 反向迭代器的 `++` 是向容器头部移动，`--` 是向尾部移动。

---

## 新手要点（和 C 的区别）

| 维度 | C 反向遍历 | C++ reverse_iterator | 为什么 |
|------|-----------|---------------------|--------|
| 遍历 | `for (i=n-1; i>=0; --i)` | `rbegin()` → `rend()` | 迭代器抽象 |
| 位置转换 | 直接用索引 | `base()` 有偏移 | 设计对称性 |
| off-by-one | 手动管理 | base() 的偏移陷阱 | 需要注意 |

**一句话：** C 用索引反向遍历，位置转换直观。C++ 的 `reverse_iterator` 的 `base()` 有 off-by-one 偏移——删除元素时要 `base() - 1`。

---

## HFT 关联

- **反向遍历找最新 tick**：`rbegin()` 从最新 tick 开始遍历，找到第一个满足条件的就停止。
- **base() 偏移陷阱**：删除反向查找的元素时务必 `base() - 1`，否则删错元素。

---

## 代码自测

### Q1: base() 偏移
```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
auto rit = std::find(v.rbegin(), v.rend(), 3);
// rit.base() 指向哪个元素？
```

<details>
<summary>答案</summary>

`rit.base()` 指向 **4**（3 的下一个元素）。

```
[1]  [2]  [3]  [4]  [5]
           ^rit ^base
```

反向迭代器 `rit` 指向 3，`base()` 转换为正向迭代器后指向 4（偏后一个）。
</details>

### Q2: 删除元素
```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
auto rit = std::find(v.rbegin(), v.rend(), 3);
v.erase(rit.base() - 1);
// v 的内容？
```

<details>
<summary>答案</summary>

v = {1, 2, 4, 5}。`rit.base() - 1` 指向 3，`erase` 删除 3。

如果用 `rit.base()`（不减 1），会删除 4——经典 off-by-one。
</details>

### Q3: 反向遍历
```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
for (auto it = v.rbegin(); it != v.rend(); ++it)
    std::cout << *it << ' ';
```
> 输出什么？

<details>
<summary>答案</summary>

输出 `5 4 3 2 1`。`rbegin()` 指向最后一个元素（5），`++it` 向头部移动（4, 3, 2, 1），`rend()` 指向第一个元素之前。
</details>

### Q4: insert 位置
```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
auto rit = std::find(v.rbegin(), v.rend(), 3);
v.insert(rit.base(), 99);
// v 的内容？
```

<detailf>
<summary>答案</summary>

v = {1, 2, 3, 99, 4, 5}。`insert(pos, val)` 在 `pos` 前插入。`rit.base()` 指向 4，所以在 4 前面（3 后面）插入 99。

```
[1] [2] [3] [99] [4] [5]
            ^insert位置
```
</details>

---

## 参考与延伸

- 上一节：[Item 26 const_iterator](item26-prefer-const-iterator.md)
- 下一节：[Item 29 流迭代器](item29-stream-iterators.md)
- 回到：[第 4 章 迭代器](README.md)
