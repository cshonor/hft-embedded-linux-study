# Item 42：优先 emplace 而非 insert

> 第 8 章 微调 · Item 42 · 下一节：[Item 41 按值传递](item41-pass-by-value.md)

## 为什么要学这个（先建立直觉）

C 程序员往数组里加元素的方式：

```c
struct Widget { int x; int y; };
struct Widget arr[100];
int n = 0;

// 先构造临时对象，再拷贝进数组
struct Widget tmp = {42, 100};
arr[n++] = tmp;  // 拷贝

// 或者直接初始化
arr[n].x = 42;
arr[n].y = 100;
n++;
```

C++ 的 `push_back` 类似 C 的第一种方式——先构造临时对象，再移动/拷贝进容器：

```cpp
v.push_back(Widget(42, 100));  // 构造临时 Widget → 移动进 v
```

`emplace_back` 类似 C 的第二种方式——直接在容器的内存里构造对象，不需要临时对象：

```cpp
v.emplace_back(42, 100);  // 直接在 v 的内存里构造 Widget(42, 100)
// 省去了临时对象的构造 + 移动
```

---

## 这节讲什么

`emplace_back`/`emplace` 在容器内**直接构造**元素，省去临时对象 + 移动/拷贝。但要注意异常安全和资源管理。

---

## 核心对比

```cpp
v.push_back(Widget(42));     // 构造临时 Widget → 移动进 v
v.emplace_back(42);          // 直接在 v 的内存里构造 Widget
```

`emplace` 的优势：无临时对象、无移动、可传任意构造参数。

### emplace 省了什么

```cpp
std::vector<std::string> v;

// push_back：构造临时 string + 移动
v.push_back(std::string("hello"));  // 1. 构造临时 string("hello")
                                     // 2. 移动临时 string 进 v
                                     // 3. 析构临时 string

// emplace_back：直接构造
v.emplace_back("hello");  // 直接在 v 的内存里构造 string("hello")
                           // 省了临时对象的构造+移动+析构

// 更明显的例子：多参数构造
std::vector<Widget> vw;
vw.push_back(Widget(1, 2, 3));  // 构造临时 + 移动
vw.emplace_back(1, 2, 3);       // 直接构造，传构造参数
```

### emplace 的限制

```cpp
// 1. 依赖 value_type 可直接构造——转换路径不明确时可能选错构造函数
std::vector<std::regex> vr;
vr.emplace_back(nullptr);  // 可能匹配意外的构造函数！

// 2. 异常安全：emplace_back(new Widget) 有泄漏风险
std::vector<std::unique_ptr<Widget>> v;
v.emplace_back(new Widget);  // 如果 vector 扩容抛异常 → 裸指针泄漏！
v.push_back(std::make_unique<Widget>());  // 安全

// 3. 与 push_back 的明确转换不同
v.push_back(nullptr);  // 明确：push_back(unique_ptr(nullptr))
v.emplace_back(nullptr);  // 不明确：调哪个构造函数？
```

---

## 常见错误（新手踩坑）

**错误 1：emplace_back 传裸 new 导致异常泄漏**
```cpp
std::vector<std::unique_ptr<Widget>> v;
v.emplace_back(new Widget);  // 扩容抛异常 → Widget 泄漏！
```
**修正：** `v.push_back(std::make_unique<Widget>());` 或 `v.emplace_back(std::make_unique<Widget>());`

**错误 2：emplace 传了不必要的临时对象**
```cpp
v.emplace_back(Widget(42));  // 和 push_back 一样——还是构造了临时对象！
```
**修正：** `v.emplace_back(42);`——传构造参数，不是构造好的对象。

**错误 3：emplace 选错构造函数**
```cpp
std::vector<std::string> v;
v.emplace_back('a', 10);  // 想构造 10 个 'a' 的 string
// 但可能匹配了意外的构造函数——需要检查 string 的构造函数列表
```
**修正：** 不确定时用 `push_back(std::string(10, 'a'))` 明确构造。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 添加元素 | `arr[n++] = tmp` | `push_back` 或 `emplace_back` | C++ 容器 |
| 临时对象 | 总是需要 | `emplace` 不需要 | 直接构造 |
| 异常安全 | 手动管理 | `make_unique` + `push_back` | RAII |
| 构造参数 | 不适用 | `emplace(ctor_args...)` | 完美转发 |

**一句话总结：** C 程序员记住——`emplace_back` 直接在容器内存里构造对象，省去临时对象。但传 `unique_ptr` 时用 `push_back(make_unique<T>())` 更安全。

---

## HFT 关联

- **行情队列**：`vector<Tick>` 批量入队用 `emplace_back` 省去临时 `Tick` 的构造 + 移动——含 `string` symbol 字段时微秒级收益可观。
- **异常安全规则**：`emplace_back` 传智能指针/值类型，不传裸 `new` 结果。
- **订单簿构造**：`order_book.emplace(exchange_id, std::move(handler))` 直接在 map 里构造，避免临时 pair。

---

## 自测题

1. `emplace_back(42)` 相比 `push_back(Widget(42))` 省掉了什么？
2. `v.emplace_back(new Widget)` 在扩容抛异常时会发生什么？正确写法是什么？
3. 什么场景下 `emplace` 反而不如 `push_back`？
4. 为什么 `push_back(make_unique<T>())` 是 `unique_ptr` 容器插入的最安全写法？
5. 下面代码有什么问题？
```cpp
std::vector<std::string> v;
v.emplace_back(std::string("hello"));
```

<details>
<summary>参考答案</summary>

1. 省掉**一个临时对象的构造 + 移动构造 + 析构**。`push_back(Widget(42))` 的流程是：先构造临时 `Widget(42)`，再把它作为右值**移动**进容器，最后析构临时对象；而 `emplace_back(42)` 把实参 `42` 完美转发给 `Widget` 的构造函数，直接在容器尾部的**未初始化存储上原地构造**（placement new），中间不产生临时对象，因此少一次移动构造和一次析构。注意它省的是"移动"，不是"拷贝"——`push_back` 传右值时本来也不会拷贝。

2. 会**泄漏那块 `new` 出来的内存**。实参 `new Widget` 在调用 `emplace_back` **之前**就先求值，得到一个裸指针；随后 `emplace_back` 内部可能因容器扩容而分配内存，若这次分配抛 `std::bad_alloc`（或元素构造抛异常），那个裸指针还没有被任何 `unique_ptr` 接管，就永远丢失了——对象泄漏。正确写法是先把裸指针交给 RAII 对象，再插入：
```cpp
v.push_back(std::make_unique<Widget>());   // 或 v.emplace_back(std::make_unique<Widget>());
```
`make_unique` 在 `push_back` 之前就完成了所有权接管，即使后续扩容抛异常，临时 `unique_ptr` 也会在栈展开时正常析构并释放对象。

3. 常见三类：①**实参已经是元素类型**（`emplace_back(std::string("hello"))`）——它只是转发给移动构造，与 `push_back` 完全等价、毫无收益。②**需要隐式转换的场景**：`emplace_back` 做的是直接初始化，会调用 `explicit` 构造函数，可能构造出你没想到的对象（如 `std::vector<std::string> v; v.emplace_back(nullptr);` 会用 `nullptr` 构造 `string`，是未定义行为；而 `push_back(nullptr)` 直接编译失败，反而更安全）。③**需要异常安全的资源类**：插入智能指针时 `push_back(std::make_unique<T>())` 比 `emplace_back(new T)` 安全（见第 2 题）。另外，`emplace` 的实参是引用传递，若传入的是容器自身的元素引用，扩容后可能引用已失效的存储——也不如先拷贝再 `push_back` 稳妥。

4. 因为它同时满足两点：①**异常安全**——`make_unique<T>()` 在调用 `push_back` 之前就构造好了 `unique_ptr`，对象已被 RAII 接管；即使 `push_back` 内部扩容分配失败抛异常，这个临时 `unique_ptr` 也会在栈展开时析构并释放对象，不存在"裸指针没人管"的窗口。②**写法合法且简洁**——`push_back(new T)` 在 `vector<unique_ptr<T>>` 上根本编译不过（`unique_ptr` 的裸指针构造函数是 `explicit`，`push_back` 需要的是隐式转换），必须先包成 `unique_ptr`。而 `emplace_back(new T)` 虽然能编译（emplace 是直接初始化，允许 explicit），却有第 2 题的泄漏风险。所以 `push_back(std::make_unique<T>())` 是最安全的写法。

5. 没有正确性 bug，但**白白放弃了 emplace 的收益，还多敲了代码**。`std::string("hello")` 先在调用点构造一个临时 string，`emplace_back` 再把实参转发给 `string` 的移动构造函数——等价于 `push_back(std::string("hello"))`，仍有"临时对象构造 + 移动 + 析构"。要真正原地构造，应直接传构造 `string` 所需的实参：
```cpp
v.emplace_back("hello");   // 用 const char* 直接在容器内构造 string，省掉临时对象
```
或者写 `v.push_back("hello");`（同样省掉显式临时对象，可读性也更好）。

</details>

---

## 参考与延伸

- 下一章：[第 9 章（无）—— 本书结束](../README.md)
- 回到：[第 8 章 微调](README.md)
