# Item 13：优先 const_iterator 而非 iterator

> 第 3 章 移步现代 C++ · Item 13 · 上一节：[Item 12 override](item12-override.md)

## 为什么要学这个（先建立直觉）

C 程序员用 `const` 指针防止意外修改，这是刻在肌肉记忆里的习惯：

```c
int arr[] = {1, 2, 3, 4, 5};
const int* p = arr;          // 指向 const 的指针——不能通过 p 改元素
// *p = 10;                  // 编译失败
for (const int* it = arr; it != arr + 5; ++it)
    printf("%d\n", *it);     // 只读遍历，安全
```

C++ 的迭代器就是"面向对象的指针"，同样分 `iterator`（可改）和 `const_iterator`（只读）。区别只在于：C++11 之前你很难**方便地**拿到 `const_iterator`——非 const 容器上 `begin()` 返回的是 `iterator`，你得先把容器转成 `const&`，或者写一长串 `std::vector<Order>::const_iterator`。结果就是大家为了省事一律 `begin()`，代码里到处是"其实根本没想改，但权限给满了"的遍历。

C++11 补上了成员函数 `cbegin()`/`cend()`/`crbegin()`/`crend()`，C++14 又补上了**非成员**版本 `std::cbegin(c)`——后者对原生数组和泛型容器也统一适用。配合 `auto`，拿 `const_iterator` 的成本终于降到零。这就是"最低权限原则"：**你不需要写权限，就别拿写权限**。多给的权限不会让代码跑得更快，只会在某天变成"谁动了我的订单簿"的线上事故。

---

## 这节讲什么

1. 四个迭代器家族成员的对照表，全部用 `static_assert` 实测类型；
2. `const_iterator` vs `const iterator`——C++ 新手第一号混淆点；
3. C++11 起 `insert`/`erase` 接受 `const_iterator`（跨标准实测）；
4. `iterator → const_iterator` 的单向转换；
5. 非成员 `std::cbegin`/`std::cend`（C++14）与 C++11 下的报错原文。

---

## 四大家族 + 单向转换（static_assert 实测）

| 含义 | 正向 | 反向 | 获取方式（成员函数） |
|------|------|------|-------------------|
| 可改元素 | `iterator` | `reverse_iterator` | `begin()` / `rbegin()` |
| 只读元素 | `const_iterator` | `const_reverse_iterator` | `cbegin()` / `crbegin()` |

把类型钉死（clang 23.1.0，`-std=c++17 -Wall` 实测通过）：

```cpp
#include <vector>
#include <type_traits>
using std::is_same_v;
using std::is_convertible_v;

int main() {
    std::vector<int> v = {1, 2, 3};
    using It   = std::vector<int>::iterator;
    using CIt  = std::vector<int>::const_iterator;
    using RIt  = std::vector<int>::reverse_iterator;
    using CRIt = std::vector<int>::const_reverse_iterator;
    static_assert(is_same_v<decltype(v.begin()),   It>);
    static_assert(is_same_v<decltype(v.cbegin()),  CIt>);
    static_assert(is_same_v<decltype(v.rbegin()),  RIt>);
    static_assert(is_same_v<decltype(v.crbegin()), CRIt>);
    // 单向转换：iterator → const_iterator 可以，反向不行
    static_assert(is_convertible_v<It, CIt>);
    static_assert(!is_convertible_v<CIt, It>);
    // 非 const 容器上，auto 只会如实接住 iterator，不会替你收紧权限
    auto it = v.begin();
    static_assert(is_same_v<decltype(it), It>);
    return 0;
}
```

最后两行是重点：**非 const 容器上 `auto it = v.begin()` 拿到的是 iterator**。`auto` 只负责少打字，不会替你表达只读意图——这就是必须写 `cbegin()` 的核心理由。反向若强行写（`std::vector<int>::iterator it = v.cbegin();`），clang 报 `error: no viable conversion from 'const_iterator' to 'iterator'`。

---

## const_iterator ≠ const iterator

新手第一号混淆点，二者 const 修饰的**层级**完全不同：

```cpp
std::vector<int> v = {1, 2, 3};

// const_iterator：指向 const 元素 —— 不能改元素，但能 ++ 移动
std::vector<int>::const_iterator cit = v.cbegin();
// *cit = 10;   // 编译失败：read-only variable is not assignable
++cit;          // OK

// const iterator：迭代器本身是 const —— 不能移动，但能改元素
const std::vector<int>::iterator it = v.begin();
*it = 10;       // OK！和你想的"只读"完全相反
// ++it;        // 编译失败
```

`++it` 的实测报错原文：

```
error: cannot increment value of type 'const std::vector<int>::iterator' (aka 'const __wrap_iter<int *>')
```

记忆口诀：`const_iterator` 修的是**元素**，`const iterator` 修的是**指针变量**；对应到 C 就是 `const int*` 与 `int* const` 的区别。

---

## C++11 起 insert / erase 接受 const_iterator

C++11 之前 `erase(iterator)` 只接受 `iterator`，于是"先 `find` 定位、再删掉"这个最常见的组合逼着你在两种迭代器间来回倒腾。C++11 把这两个成员改成接受 `const_iterator`。实测（`-std=c++11` 编译并运行）：

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};
    auto pos = std::find(v.cbegin(), v.cend(), 30);   // const_iterator
    auto after = v.erase(pos);                       // C++11 起 OK
    std::cout << "after erase, *after = " << *after << "\n";
    auto pos2 = std::find(v.cbegin(), v.cend(), 40);
    auto inserted = v.insert(pos2, 35);              // C++11 起 OK
    std::cout << "after insert, *inserted = " << *inserted << "  ";
    for (auto x : v) std::cout << x << ' ';
    std::cout << "\n";
    auto it = v.cbegin();
    // *it = 99;   // 编译失败：read-only variable is not assignable
    return 0;
}
```

真实输出：

```
after erase, *after = 40
after insert, *inserted = 35  10 20 35 40 50
```

`erase`/`insert` 的返回类型跟传入的迭代器一致——传 `const_iterator` 就返回 `const_iterator`，整条链路可以一路只读到底。

---

## 非成员 cbegin/cend（C++14）

成员函数版 `cbegin()` 是 C++11 的；**非成员** `std::cbegin(c)` / `std::cend(c)` 是 C++14 的。差别在泛型代码和原生数组：

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
auto it  = std::cbegin(v);        // C++14 起 OK
int arr[] = {7, 8, 9};
auto ait = std::cbegin(arr);       // 原生数组也吃得下（成员函数版做不到）
```

C++14 实测输出：

```
*std::cbegin(v) = 1
*std::cbegin(arr) = 7
```

同样的代码用 `-std=c++11` 编译，报错原文：

```
error: no member named 'cbegin' in namespace 'std'; did you mean 'begin'?
note: 'begin' declared here
error: no matching function for call to 'begin'
note: candidate template ignored: could not match 'initializer_list' against 'std::vector'
```

C++14 那一批非成员访问函数是一整套：`std::cbegin/cend/rbegin/rend/crbegin/crend`。写泛型工具函数时优先用非成员版本——它对容器、原生数组、第三方容器一视同仁。

---

## 完整可运行例子

把上面的 `mean` / `mean_of` 与迭代器行为放在一起跑（`-std=c++14`）：

```cpp
#include <iostream>
#include <vector>
#include <numeric>
#include <iterator>

// 只读算法：承诺写进签名
double mean(std::vector<double>::const_iterator first,
            std::vector<double>::const_iterator last) {
    if (first == last) return 0.0;
    return std::accumulate(first, last, 0.0) / std::distance(first, last);
}

// 泛型版：非成员 cbegin，容器与定长数组都通用
template <typename C>
double mean_of(const C& c) {
    auto first = std::cbegin(c);
    auto last  = std::cend(c);
    if (first == last) return 0.0;
    return std::accumulate(first, last, 0.0) / std::distance(first, last);
}

int main() {
    std::vector<double> px = {100.5, 101.0, 99.5, 102.25};
    std::cout << "mean(px)     = " << mean(px.cbegin(), px.cend()) << "\n";
    std::cout << "mean_of(px)  = " << mean_of(px) << "\n";
    double arr[] = {1.0, 2.0, 3.0, 4.0};
    std::cout << "mean_of(arr) = " << mean_of(arr) << "\n";
    // 非 const 容器上 begin() 拿到的是 iterator，可以改元素
    auto mutable_it = px.begin();
    *mutable_it = 999.0;
    std::cout << "px[0]        = " << px[0] << "\n";
    // 单向转换：iterator → const_iterator 可以
    std::vector<double>::const_iterator cit = px.begin();
    std::cout << "*cit         = " << *cit << "\n";
    return 0;
}
```

clang 23.1.0 实测输出：

```
mean(px)     = 100.812
mean_of(px)  = 100.812
mean_of(arr) = 2.5
px[0]        = 999
*cit         = 999
```

注意 `mean_of(px)` 与 `mean_of(arr)` 用的是同一份代码——非成员 `std::cbegin` 抹平了容器与原生数组的差异，这正是 C++14 补齐它的意义。

---

## 常见错误（新手踩坑）

1. **混淆 `const_iterator` 和 `const iterator`**。写 `const auto it = v.begin();` 以为拿到了只读迭代器，结果 `*it = 10` 合法、`++it` 非法——恰好搞反。想只读就用 `v.cbegin()`。
2. **以为 C++11 的非成员 `std::cbegin` 可用**。非成员版本是 C++14 的；`-std=c++11` 下报 `error: no member named 'cbegin' in namespace 'std'`。成员函数版 `v.cbegin()` 才是 C++11 的。
3. **把 `cbegin()` 的结果拿去改元素**：`*it = 42` 报 `error: read-only variable is not assignable`。要改就用 `v.begin()`。
---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 只读指针 | `const int* p` | `const_iterator`（`v.cbegin()`） | 表达"不修改"意图 |
| 指针本身不可动 | `int* const p` | `const iterator`（很少用） | 与上面是两码事 |
| 遍历方式 | 指针 + 偏移 | 迭代器 + `++`/`*` | 面向对象抽象 |
| 算法支持 | `qsort`（函数指针） | `std::sort`（迭代器 + 仿函数） | 泛型编程 |

**一句话总结：** `const_iterator` ↔ C 的 `const int*`（指向 const 的指针）；`const iterator` ↔ `int* const`（指针本身是 const）。只读遍历用 `cbegin()`/`cend()`，范围 for 用 `for (const auto& x : v)`。

---

## HFT 关联

- **只读遍历行情**：`for (auto it = ticks.cbegin(); it != ticks.cend(); ++it)` 明确表达"只读"，任何写入都被编译器拦下。订单簿快照分发给多个策略时尤其重要。
- **多策略共享数据**：多个策略线程并发读同一份行情快照，用 `const_iterator` 确保不会有策略意外改数据——一旦有人写，编译期就失败，而不是变成一次难复现的数据竞争（另见 [Item 16](item16-const-thread-safety.md)）。
- **只读算法**：`std::find`、`std::count`、`std::accumulate` 等传 `cbegin()`/`cend()`；泛型工具函数用非成员 `std::cbegin`，容器与定长数组都通用。

---

## 自测题

**1. `const_iterator` 和 `const iterator` 有什么区别？**
<details><summary>答案</summary>
`const_iterator` 是**指向 const 元素**的迭代器：解引用得到 `const T&`，不能改元素，但可以 `++` 移动。`const iterator` 是**迭代器对象本身 const**：不能 `++it`，但 `*it = 10` 合法。二者 const 修饰的层级不同——一个修元素，一个修迭代器。对应 C：`const_iterator` ↔ `const int*`，`const iterator` ↔ `int* const`。
</details>

**2. 非 const 容器上 `auto it = v.begin()` 和 `auto it = v.cbegin()` 分别得到什么类型？**
<details><summary>答案</summary>
分别是 `std::vector<T>::iterator` 和 `std::vector<T>::const_iterator`。实测 `static_assert(is_same_v<decltype(v.begin()), It>)` 通过。关键在于 `auto` 只会如实接住 `begin()` 的返回类型，不会替你收紧权限——想表达只读必须写 `cbegin()`。
</details>

**3. C++11 和 C++14 在 `const_iterator` 支持上分别补了什么？**
<details><summary>答案</summary>
C++11：①容器成员函数 `cbegin()/cend()/crbegin()/crend()`；②`insert`/`erase` 改为接受 `const_iterator`。C++14：非成员版本 `std::cbegin/cend/rbegin/rend/crbegin/crend`，对原生数组和任何提供 `begin()/end()` 的容器都适用——泛型代码的通用性在 C++14 才补齐。实测 `-std=c++11` 下 `std::cbegin` 报 `no member named 'cbegin' in namespace 'std'`。
</details>

**4. `iterator` 和 `const_iterator` 之间能互相转换吗？方向如何？**
<details><summary>答案</summary>
单向：`iterator → const_iterator` 可隐式转换（加权限安全），反向不行（去掉权限不安全）。实测 `static_assert(is_convertible_v<It, CIt>)` 与 `static_assert(!is_convertible_v<CIt, It>)` 都通过。实际意义：函数参数写 `const_iterator` 不会给调用方带来负担。
</details>

**5. 为什么推荐用 `cbegin()`/`cend()` 而非 `begin()`/`end()`？**
<details><summary>答案</summary>
因为 `begin()` 的返回类型取决于容器的常量性：非 const 容器上返回 `iterator`，后续任何人都可以通过它改元素，与"只读遍历"的意图相悖。`cbegin()` 无条件返回 `const_iterator`，把"不修改"写进类型、由编译器强制检查，且不依赖容器本身是否 `const`。在 `auto it = c.begin();` 这种写法下尤其重要。
</details>

**6. 下面代码有什么问题？**
```cpp
std::vector<int> v = {1, 2, 3};
const auto it = v.begin();
*it = 10;
++it;
```
<details><summary>答案</summary>
`++it` 编译失败。实测报错：`error: cannot increment value of type 'const std::vector<int>::iterator' (aka 'const __wrap_iter<int *>')`。`const auto it = v.begin();` 中 `auto` 推为 `std::vector<int>::iterator`，`const` 修饰的是迭代器自身，所以 `++it` 非法；而 `*it = 10` 反而合法——const 的是迭代器不是元素。正确写法：只读遍历 `auto it = v.cbegin();`，改元素 `auto it = v.begin();`。
</details>

**7. 为什么 C++14 的非成员 `std::cbegin` 对泛型代码是"补齐"而非"语法糖"？**
<details><summary>答案</summary>
因为成员函数版 `c.begin()` 只能用于类类型容器，原生数组没有成员函数，泛型模板里还得为数组单独写一份重载。非成员 `std::cbegin(c)` 对 `std::vector` 和 `double arr[4]` 用同一行代码就能拿到 `const_iterator`——实测 `mean_of(px)` 与 `mean_of(arr)` 共用一份实现，分别输出 `100.812` 和 `2.5`。写通用工具函数时优先用非成员版本。
</details>

---

## 参考与延伸

- 下一节：[Item 14 noexcept](item14-noexcept.md)
- 回到：[第 3 章 移步现代 C++](README.md)
