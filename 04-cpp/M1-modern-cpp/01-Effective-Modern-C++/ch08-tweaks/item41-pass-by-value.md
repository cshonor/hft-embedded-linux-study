# Item 41：可拷贝参数考虑按值传递

> 第 8 章 微调 · Item 41 · 下一节：[Item 42 emplace vs insert](item42-emplace-vs-insert.md)

## 为什么要学这个（先建立直觉）

C 的参数传递很简单——要么按值（拷贝），要么传指针：

```c
// C：按值传递（拷贝）
void process(int x) { ... }

// C：传指针（不拷贝）
void process_large(const BigStruct* s) { ... }
```

C++ 有了引用和移动语义，参数传递方式变多了。传统 C++ 做法：

```cpp
// 传统：const 引用 + 拷贝
void add(const std::string& name) {
    names.push_back(name);  // 1 次拷贝
}

// 加右值重载：避免拷贝
void add(std::string&& name) {
    names.push_back(std::move(name));  // 1 次移动
}
```

但如果函数**总是要拷贝**参数，且移动廉价，可以简化为按值传递：

```cpp
// 按值传递 + move 进容器
void add(std::string name) {
    names.push_back(std::move(name));  // 左值：拷贝+移动；右值：移动+移动
}
```

一个函数替代两个重载，代码更简洁。

---

## 这节讲什么

对于"会被拷贝的可拷贝形参"，传统做法是传 `const T&` 再拷贝。但如果函数总是要拷贝，且移动廉价，按值传递能简化代码。

---

## 三种方式对比

```cpp
// 方式 1：两个重载（最优性能，代码多）
void add(const T& x) { v.push_back(x); }           // 左值：1 次拷贝
void add(T&& x)      { v.push_back(std::move(x)); } // 右值：1 次移动

// 方式 2：按值传递（代码少，左值多一次移动）
void add(T x) { v.push_back(std::move(x)); }        // 左值：拷贝构造形参 + 移动
                                                     // 右值：移动构造形参 + 移动

// 方式 3：万能引用 + forward（最优性能，但有限制）
template<class T> void add(T&& x) { v.push_back(std::forward<T>(x)); }
// 万能引用有 Item 26 的重载问题
```

**代价分析：**

| 实参类型 | 两个重载 | 按值传递 | 万能引用 |
|---------|---------|---------|---------|
| 左值 | 1 次拷贝 | 1 次拷贝 + 1 次移动 | 1 次拷贝 |
| 右值 | 1 次移动 | 2 次移动 | 1 次移动 |

按值传递对左值多了一次移动——只有当**移动廉价**时才值得。

**适用条件：** ①函数总是拷贝形参；②移动廉价（`string`、`vector`、智能指针）；③拷贝与移动代价相近。

不满足任一条，用重载或万能引用 + `forward`。

---

## 常见错误（新手踩坑）

**错误 1：移动昂贵的类型按值传递**
```cpp
void add(std::array<int, 1000> arr) {  // 按值——array 的移动是 O(N)！
    v.push_back(std::move(arr));  // O(N) 移动
}
// 正确：用 const 引用
void add(const std::array<int, 1000>& arr) {
    v.push_back(arr);  // 1 次拷贝，和按值的总代价相同但更清晰
}
```
**修正：** 移动昂贵的类型用 `const T&` + 拷贝。

**错误 2：不需要拷贝时按值传递**
```cpp
void process(Widget w) {  // 按值——但只是读取 w，不需要拷贝！
    use(w);  // 白白拷贝了一次
}
```
**修正：** 只读参数用 `const Widget&`。

**错误 3：按值传递但忘了 move 进容器**
```cpp
void add(std::string name) {
    names.push_back(name);  // 忘了 std::move！→ 拷贝而非移动
}
```
**修正：** `names.push_back(std::move(name));`

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 按值传递 | 总是拷贝 | 拷贝构造形参 | 相同 |
| 引用传递 | 传指针 | `const T&` | C++ 有引用 |
| 移动语义 | 不存在 | 按值 + `move` | C++11 |
| 重载简化 | 不适用 | 按值替代两个重载 | 代码简洁 |

**一句话总结：** C 程序员记住——C++ 的"总是要拷贝的参数"用按值传参 + `std::move` 进容器，一个函数替代两个重载。但只对移动廉价的类型（`string`/`vector`/智能指针）划算。

---

## HFT 关联

- **策略配置注册**：`string` symbol 移动廉价，配置函数用按值传参 + `std::move` 进成员，代码简洁且性能可接受。
- **避免不必要的拷贝**：`void add(std::string name)` 对左值多一次移动——如果 `string` 很长（如 JSON 配置），考虑用 `const string&` + 右值重载。
- **移动代价意识**：`array<T, N>` 的移动是 O(N)——HFT 代码中避免对 `array` 按值传递。

---

## 自测题

1. 按值传递可拷贝形参相比 `const T&` + 拷贝，对左值实参多出了什么代价？
2. 什么条件下按值传递才划算？
3. 移动昂贵的类型应该用什么方式传参？
4. 下面代码有什么问题？
```cpp
void add(std::string name) {
    names.push_back(name);
}
```

---

## 参考与延伸

- 下一节：[Item 42 emplace vs insert](item42-emplace-vs-insert.md)
- 回到：[第 8 章 微调](README.md)
