# Item 13：优先 const_iterator 而非 iterator

> 第 3 章 移步现代 C++ · Item 13 · 上一节：[Item 12 override](item12-override.md)

## 为什么要学这个（先建立直觉）

C 程序员用指针遍历数组时，`const` 指针防止意外修改：

```c
int arr[] = {1, 2, 3, 4, 5};
const int* p = arr;          // 指向 const 的指针——不能通过 p 修改元素
// *p = 10;                  // 编译失败！p 是 const 指针
for (const int* it = arr; it != arr + 5; ++it)
    printf("%d\n", *it);     // 只读遍历，安全
```

C++ 的迭代器是"面向对象的指针"。和 C 指针一样，有 `iterator`（可修改）和 `const_iterator`（只读）之分。C++11 引入了 `cbegin()`/`cend()` 让你轻松拿到 `const_iterator`，配合 `auto` 自动推导类型。

**为什么重要：** 在团队协作和代码审查中，`const_iterator` 表达了"我承诺不修改容器内容"的意图。编译器帮你检查——如果不小心写了 `*it = newValue`，编译失败。这在 HFT 场景中尤其关键——只读遍历行情数据时，意外修改会导致严重 bug。

---

## 这节讲什么

C++11 引入 `cbegin()`/`cend()`，配合 `auto` 拿到 `const_iterator`，防止意外修改。STL 算法在 C++14 起支持 `const_iterator`。

---

## 核心用法

### cbegin/cend 配合 auto

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

// C++11：用 cbegin/cend 拿 const_iterator
auto it = std::find(v.cbegin(), v.cend(), 3);  // it 是 const_iterator
// *it = 10;  // 编译失败！const_iterator 不能修改元素

// C++14：STL 算法支持 const_iterator 并返回同类型
auto it2 = std::find(v.cbegin(), v.cend(), 3);  // 返回 const_iterator（C++14）
// C++11 的 std::find 虽然能编译，但返回的是普通 iterator——不太理想
```

### const_iterator vs const iterator

```cpp
std::vector<int> v = {1, 2, 3};

// const_iterator：指向 const 元素的迭代器（不能通过它修改元素）
std::vector<int>::const_iterator cit = v.cbegin();
// *cit = 10;  // 编译失败！指向的元素是 const

// const iterator：迭代器本身是 const（不能改变指向，但可以修改元素）
const std::vector<int>::iterator it = v.begin();
*it = 10;       // OK！可以修改元素
// ++it;        // 编译失败！迭代器本身是 const
```

### 范围 for 循环中的 const

```cpp
std::vector<Tick> ticks = get_ticks();

// 只读遍历——用 const 引用
for (const auto& tick : ticks) {
    std::cout << tick.price << "\n";
    // tick.price = 0;  // 编译失败！const 引用
}

// 需要修改——用非 const 引用
for (auto& tick : ticks) {
    tick.normalize();  // OK，可以修改
}
```

C++14 起算法接受 `const_iterator` 并返回同类型，C++11 的部分算法只接受 `iterator`。

---

## 常见错误（新手踩坑）

**错误 1：混淆 const_iterator 和 const iterator**
```cpp
std::vector<int> v = {1, 2, 3};
const std::vector<int>::iterator it = v.begin();  // const iterator，不是 const_iterator
*it = 10;   // OK！能改元素——和你想的"只读"相反
```
**修正：** 用 `v.cbegin()` 拿 `const_iterator`，或用 `const auto&` 范围 for。

**错误 2：C++11 中算法不接受 const_iterator**
```cpp
std::vector<int> v = {3, 1, 4, 1, 5};
// C++11：std::sort(v.cbegin(), v.cend());  // 可能编译失败
// C++14：std::sort(v.cbegin(), v.cend());  // OK
```
**修正：** C++11 中需要修改的算法用 `begin()`/`end()`，只读的用 `cbegin()`/`cend()`。

**错误 3：用了 const_iterator 后想修改元素**
```cpp
auto it = v.cbegin();
if (it != v.cend()) {
    *it = 42;  // 编译失败！
}
```
**修正：** 要修改就用 `v.begin()`，要只读才用 `v.cbegin()`。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 只读指针 | `const int* p` | `const_iterator`（`v.cbegin()`） | 表达"不修改"意图 |
| 遍历方式 | 指针 + 偏移 | 迭代器 + `++`/`*` | 面向对象抽象 |
| 算法支持 | `qsort`（函数指针） | `std::sort`（迭代器+仿函数） | 泛型编程 |
| 只读遍历 | `const int* it = arr;` | `for (const auto& x : v)` | 范围 for + auto 更简洁 |

**一句话总结：** C 程序员记住——`const_iterator` 等价于 C 的 `const int*`（指向 const 的指针），用 `cbegin()`/`cend()` 获取。只读遍历用 `for (const auto& x : v)` 最简洁。

---

## HFT 关联

- **只读遍历行情**：`for (auto it = ticks.cbegin(); it != ticks.cend(); ++it)` 表达"只读"意图，编译器帮你检查。
- **多策略共享数据**：多个策略线程并发读同一份行情快照，用 `const_iterator` 确保不会有策略意外修改数据。
- **STL 算法只读操作**：`std::find`、`std::count`、`std::accumulate` 等只读算法传 `cbegin()`/`cend()`，明确表达不修改意图。

---

## 自测题

1. `const_iterator` 和 `const iterator` 有什么区别？
2. C++11 和 C++14 对 `const_iterator` 在 STL 算法中的支持有何不同？
3. 为什么推荐用 `cbegin()`/`cend()` 而非 `begin()`/`end()`？
4. `for (const auto& x : v)` 和 `for (auto& x : v)` 什么时候用哪个？
5. 下面代码有什么问题？
```cpp
std::vector<int> v = {1, 2, 3};
const auto it = v.begin();
*it = 10;
++it;
```

<details>
<summary>参考答案</summary>

1. `const_iterator` 是一个**指向 const 元素**的迭代器类型（`std::vector<T>::const_iterator`）：解引用得到 `const T&`，不能通过它修改元素，但迭代器本身可以自增。`const iterator` 是"迭代器对象本身是 const"：`const std::vector<T>::iterator it = ...`，它不能 `++it`（迭代器被 const 限定），但 `*it = 10` 修改元素是允许的。可见二者 const 修饰的对象层级不同——一个修元素，一个修迭代器。

2. C++11 起容器提供了 `cbegin()`/`cend()` 成员函数，可以直接拿到 `const_iterator`（此前只能用 `const` 容器或强制转换拿到）。C++14 进一步补充了**非成员**版本 `std::cbegin(c)` / `std::cend(c)` / `std::cbegin(arr)`，对原生数组和任何提供 `begin()/end()` 的容器（含泛型代码里的第三方容器）都能统一使用，泛型代码的通用性在 C++14 才补齐。

3. 因为 `begin()` 的返回类型取决于容器的常量性：非 const 容器上 `begin()` 返回 `iterator`，一旦后续有人通过它修改元素，与"只读遍历"的意图相悖。`cbegin()` 无条件返回 `const_iterator`，把"不修改"这一意图写进类型、由编译器强制检查，且完全不依赖容器本身是否 `const`。在 `auto it = c.begin();` 这种写法下尤其重要——`auto` 不会替你表达只读意图。

4. `const auto& x : v` 用于**只读**遍历：避免拷贝（引用），又禁止修改元素，是默认首选；`auto& x : v` 用于需要**就地修改**元素时。若元素是小标量（`int`、`double`）且不介意拷贝，`auto x : v` 也可以，但引用写法能避免大对象拷贝。注意代理类型（如 `vector<bool>`）用 `const auto&` 可能接到代理引用，需要 `static_cast<bool>`。

5. 编译不过。`const auto it = v.begin();` 中 `auto` 推为 `std::vector<int>::iterator`，`const` 修饰迭代器自身，所以 `++it` 非法（const 对象不能调用非 const 的 `operator++`）。而 `*it = 10` 反而是合法的——因为 const 的是迭代器不是元素，这是"const iterator 而非 const_iterator"的典型误用。正确写法：`auto it = v.cbegin();`（只读遍历）或 `const auto it = v.cbegin();`（连迭代器也不许动），修改元素则写 `auto it = v.begin();`。

</details>

---

## 参考与延伸

- 下一节：[Item 14 noexcept](item14-noexcept.md)
- 回到：[第 3 章 移步现代 C++](README.md)
