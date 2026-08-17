# Item 8：不存 `auto_ptr`

> 第 1 章 容器 · Item 8 · 上一节：[Item 7 容器销毁时删除指针](item07-delete-pointers-on-destroy.md) · 下一节：[Item 9 删除元素的正确方式](item09-correct-element-removal.md)

## 为什么要学这个（先建立直觉）

C 程序员没有这个问题——C 的指针拷贝就是地址拷贝，所有权靠程序员心算：

```c
Widget* p = malloc(sizeof(Widget));
Widget* q = p;  // 两个指针指向同一对象，谁负责 free？靠注释约定
```

C++98 的 `auto_ptr` 试图解决独占所有权问题，但设计有致命缺陷——**拷贝会转移所有权**：

```cpp
std::auto_ptr<Widget> p(new Widget());
std::auto_ptr<Widget> q = p;  // p 变成空指针！所有权转移到 q
// p->doSomething();  // UB：p 已经空了
```

把 `auto_ptr` 放进容器是灾难——STL 算法会拷贝元素，意外掏空原对象。

---

## 这节讲什么

`auto_ptr` 的拷贝是"转移所有权"——拷贝后原对象变空。STL 算法（`sort`/`copy`/`remove` 等）在内部拷贝元素时会意外掏空 `auto_ptr`，导致悬空指针。C++11 起 `auto_ptr` 弃用（C++17 移除），改用 `unique_ptr`（明确不可拷贝）。

---

## auto_ptr 的问题

```cpp
std::auto_ptr<Widget> p(new Widget());
std::auto_ptr<Widget> q = p;  // "拷贝" → p 变空，q 持有对象
// 这看起来像拷贝，实际是 move —— 违反拷贝语义直觉

// STL 算法拷贝元素时的问题
std::vector<std::auto_ptr<Widget>> v;
v.push_back(std::auto_ptr<Widget>(new Widget()));
v.push_back(std::auto_ptr<Widget>(new Widget()));

std::sort(v.begin(), v.end(), cmp);  // sort 内部拷贝元素
// → 某些 auto_ptr 被意外掏空！v 里有空指针！
```

### unique_ptr 的正确设计

```cpp
std::unique_ptr<Widget> p = std::make_unique<Widget>();
// std::unique_ptr<Widget> q = p;  // ❌ 编译错误——不可拷贝
std::unique_ptr<Widget> q = std::move(p);  // ✅ 显式移动，p 变空
// 移动是显式的，不会意外发生
```

---

## 常见错误（新手踩坑）

### 错误 1：在容器中使用 auto_ptr

```cpp
std::vector<std::auto_ptr<Widget>> v;  // C++11 起编译警告，C++17 编译错误
v.push_back(std::auto_ptr<Widget>(new Widget()));
std::sort(v.begin(), v.end());  // UB：sort 拷贝元素时掏空 auto_ptr
```

**修正：** 用 `vector<unique_ptr<Widget>>`。

### 错误 2：把 auto_ptr 传给函数

```cpp
void process(std::auto_ptr<Widget> p) { /* p 在函数结束时 delete */ }
std::auto_ptr<Widget> w(new Widget());
process(w);  // w 被拷贝到 p → w 变空！
// w->doSomething();  // UB
```

**修正：** 传 `unique_ptr` 时必须 `std::move`，或传引用/裸指针。

### 错误 3：在旧代码库中不升级 auto_ptr

```cpp
// 旧代码
std::auto_ptr<Widget> makeWidget() { return std::auto_ptr<Widget>(new Widget()); }
// C++17 编译错误
```

**修正：** 全局替换 `auto_ptr` → `unique_ptr`，`auto_ptr<T>(new T)` → `make_unique<T>()`。

---

## 新手要点（和 C 的区别）

| 维度 | C | C++98 auto_ptr | C++11 unique_ptr | 为什么 |
|------|---|----------------|-------------------|--------|
| 所有权 | 无 | 拷贝=转移（隐式） | 不可拷贝，显式移动 | 语义明确 |
| 容器安全 | N/A | ❌ 灾难 | ✅ 安全 | STL 算法拷贝 |
| 状态 | N/A | C++17 移除 | C++11 起 | 设计缺陷修正 |

**一句话：** C 的指针拷贝是地址拷贝（无所有权语义），`auto_ptr` 试图加所有权但拷贝语义有缺陷，`unique_ptr` 用"不可拷贝+显式移动"修正——这是 C++ 从错误中学习的典范。

---

## HFT 关联

- **`unique_ptr` 替代 `auto_ptr`**：HFT 代码库用 C++11+，全面使用 `unique_ptr`，杜绝 `auto_ptr` 的隐式转移陷阱。
- **不可拷贝 = 容器安全**：`vector<unique_ptr<T>>` 中元素不会被 STL 算法意外掏空，因为 `unique_ptr` 不可拷贝。

---

## 代码自测

### Q1: auto_ptr 的隐式转移
```cpp
std::auto_ptr<Widget> p(new Widget());
std::auto_ptr<Widget> q = p;  // "拷贝"
// p 的状态是什么？
```

<details>
<summary>答案</summary>

**p 变成空指针**。`auto_ptr` 的"拷贝"实际上是转移所有权——拷贝后源对象变空。这是 `auto_ptr` 被弃用的根本原因：拷贝语义违反直觉。

`unique_ptr` 修正了这个问题——拷贝是编译错误，只能 `std::move`。
</details>

### Q2: unique_ptr 的正确用法
```cpp
auto p = std::make_unique<Widget>();
// A
auto q = p;            // 
// B
auto q = std::move(p); //
// C
auto q = std::make_unique<Widget>(); //
```
> A/B/C 哪个能编译？

<details>
<summary>答案</summary>

- **A**：❌ 编译错误。`unique_ptr` 不可拷贝。
- **B**：✅ 移动构造，p 变空。
- **C**：✅ 独立构造新的 `unique_ptr`。

`unique_ptr` 的"不可拷贝"设计让所有权转移必须显式（`std::move`），杜绝了 `auto_ptr` 的隐式转移陷阱。
</details>

### Q3: 容器安全
```cpp
// 为什么 vector<auto_ptr<Widget>> 不安全？
std::vector<std::auto_ptr<Widget>> v;
v.push_back(std::auto_ptr<Widget>(new Widget()));
v.push_back(std::auto_ptr<Widget>(new Widget()));
std::sort(v.begin(), v.end(), cmp);
```

<details>
<summary>答案</summary>

`sort` 内部会拷贝元素（排序需要交换/移动）。`auto_ptr` 的"拷贝"是转移所有权——拷贝后源变空。sort 过程中某些 `auto_ptr` 被意外掏空，容器里出现空指针。后续解引用 → **use-after-free**。

`unique_ptr` 不可拷贝，sort 会使用移动（`std::swap` 对 `unique_ptr` 有特化），安全。
</details>

### Q4: 升级旧代码
```cpp
// 旧代码（C++03）
std::auto_ptr<Widget> createWidget() {
    return std::auto_ptr<Widget>(new Widget(args));
}
```
> 写出 C++11 的等价代码。

<details>
<summary>答案</summary>

```cpp
std::unique_ptr<Widget> createWidget() {
    return std::make_unique<Widget>(args);
}
```

替换规则：`auto_ptr<T>` → `unique_ptr<T>`，`auto_ptr<T>(new T(args))` → `make_unique<T>(args)`。
</details>

---

## 参考与延伸

- 上一节：[Item 7 容器销毁时删除指针](item07-delete-pointers-on-destroy.md)
- 下一节：[Item 9 删除元素的正确方式](item09-correct-element-removal.md)
- 回到：[第 1 章 容器](README.md)
