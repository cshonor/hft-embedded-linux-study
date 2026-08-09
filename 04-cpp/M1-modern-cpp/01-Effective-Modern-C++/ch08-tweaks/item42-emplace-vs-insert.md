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

---

## 参考与延伸

- 下一章：[第 9 章（无）—— 本书结束](../README.md)
- 回到：[第 8 章 微调](README.md)
