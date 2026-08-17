# Item 45：用 typedef 简化冗长类型

> 第 7 章 使用 STL 编程 · Item 45 · 上一节：[Item 44 头文件](item44-include-correct-headers.md) · 下一节：[Item 46 成员函数 vs 算法](item46-member-vs-algorithm.md)

## 为什么要学这个（先建立直觉）

在 C 里，`typedef` 主要用于简化结构体类型名和函数指针签名。C++ STL 的模板类型经常很长，`typedef` 的作用更大。

```c
/* C: typedef 简化函数指针 */
typedef int (*CompareFunc)(const void*, const void*);
void sort_with(void* base, size_t n, size_t sz, CompareFunc cmp);
// 不用 typedef 的话：
// void sort_with(void* base, size_t n, size_t sz,
//               int (*cmp)(const void*, const void*));
```

```cpp
// C++: STL 容器类型可能很长
std::map<int, std::pair<std::string, std::vector<double>>> data;
// 迭代器更长：
std::map<int, std::pair<std::string, std::vector<double>>>::iterator it;

// 用 typedef 简化
using DataMap = std::map<int, std::pair<std::string, std::vector<double>>>;
DataMap data;
DataMap::iterator it;  // 简洁
```

**直觉**：STL 容器类型经常嵌套，`typedef`/`using` 让类型名变短，还让"换容器只改一处"成为可能。

## 这节讲什么

### typedef vs using（C++11）

```cpp
// C++03: typedef
typedef std::map<int, std::string> IntStringMap;

// C++11: using（别名，更推荐）
using IntStringMap = std::map<int, std::string>;
// 两者等价，但 using 语法更直观，且支持模板别名

// using 支持模板别名（typedef 不支持）
template<typename T>
using Vec = std::vector<T>;  // 模板别名
Vec<int> v;  // = std::vector<int>
```

### 换容器只改一处

```cpp
// 一处定义
using OrderBook = std::map<int, Order>;  // 用红黑树
// using OrderBook = std::unordered_map<int, Order>;  // 换哈希表，只改这里

OrderBook book;                           // 自动跟随
OrderBook::iterator it = book.find(42);   // 自动跟随
for (OrderBook::value_type& kv : book) {} // 自动跟随
```

### 常见 typedef 场景

```cpp
// 1. 迭代器类型
using MapIter = std::map<int, std::string>::iterator;
using MapConstIter = std::map<int, std::string>::const_iterator;

// 2. 容器 + 比较器 + 分配器
using SortedSet = std::set<int, std::greater<int>>;

// 3. 函数类型
using Callback = std::function<void(const Tick&)>;
// C++03: typedef void(*Callback)(const Tick&);

// 4. 智能指针
using WidgetPtr = std::unique_ptr<Widget>;
using WidgetList = std::vector<WidgetPtr>;
```

## 常见错误（新手踩坑）

### 错误 1：在头文件和源文件之间类型不一致

```cpp
// header.h
using OrderMap = std::map<int, Order>;

// source.cpp
std::map<int, Order> m;  // 没用 typedef，后来换容器时漏改
```

**修复**：始终用 typedef 定义的别名。

### 错误 2：typedef 模板（C++03 无法做）

```cpp
// C++03: 无法用 typedef 定义模板别名
template<typename T>
typedef std::vector<T> Vec;  // 编译错误！

// 只能用 traits 或继承变通
template<typename T>
struct Vec { typedef std::vector<T> type; };
Vec<int>::type v;  // 繁琐
```

**修复**：用 C++11 `using`。

```cpp
template<typename T>
using Vec = std::vector<T>;
Vec<int> v;  // 简洁
```

### 错误 3：过度 typedef 导致可读性下降

```cpp
// 过度抽象
using Data = std::vector<std::pair<int, std::string>>;
using Container = Data;
using Collection = Container;
using Items = Collection;
// 读代码的人要追踪 4 层别名才知道真实类型
```

**修复**：typedef 应有意义，层数不超过 1-2 层。

## 新手要点（和 C 的区别）

| 方面 | C | C++ |
|------|---|-----|
| 主要用途 | 函数指针、结构体 | STL 容器/迭代器类型 |
| 模板别名 | 不支持 | `using`（C++11） |
| 语法 | `typedef T Name;` | `using Name = T;`（推荐） |
| 换容器 | 手动改所有声明 | 改 typedef 一处 |

## HFT 关联

- **换容器只改一处**：`using OrderBook = std::map<...>` → 切换到 `unordered_map` 只改一行
- **迭代器别名**：`using OrderBookIter = OrderBook::iterator` 简化热路径代码
- **函数类型别名**：策略回调用 `using OnTick = std::function<void(const Tick&)>` 统一接口

## 代码自测

### Q1: using vs typedef

```cpp
// A: typedef
typedef std::vector<int> IntVec1;

// B: using
using IntVec2 = std::vector<int>;
```
> A 和 B 有什么区别？

<details>
<summary>答案</summary>

**功能等价**。`IntVec1` 和 `IntVec2` 都是 `std::vector<int>` 的别名。

区别在于：
1. `using` 语法更直观（像赋值，左=右）
2. `using` 支持模板别名，`typedef` 不支持
3. C++11 起推荐用 `using`

```cpp
// using 能做的 typedef 不能
template<typename T>
using Vec = std::vector<T>;  // OK
```
</details>

### Q2: 换容器

```cpp
using Container = std::map<int, std::string>;
Container c;
Container::iterator it;
Container::value_type pair;
```
> 如果要把 map 换成 unordered_map，需要改几处？

<details>
<summary>答案</summary>

只需改 **1 处**：

```cpp
using Container = std::unordered_map<int, std::string>;
```

后续的 `Container::iterator`、`Container::value_type` 都自动跟随。

这就是 typedef/using 的核心价值——**类型抽象一处定义**。
</details>

### Q3: 模板别名

```cpp
template<typename T>
using Stack = std::vector<T>;

Stack<int> s;
s.push(1);
s.push(2);
std::cout << s.size();
```
> 输出是什么？这段代码用了什么 C++11 特性？

<details>
<summary>答案</summary>

输出 **2**。

使用了 C++11 的**模板别名**（template alias）——`using` 可以参数化，`typedef` 不能。

`Stack<int>` 等价于 `std::vector<int>`，只是换了个名字。
</details>

### Q4: 函数指针 typedef

```cpp
// C 风格
typedef void(*Callback1)(int);

// C++ 风格
using Callback2 = void(*)(int);

// std::function
using Callback3 = std::function<void(int)>;
```
> 三者在作为参数传递时有什么区别？

<details>
<summary>答案</summary>

- **Callback1 / Callback2**：等价，都是函数指针类型。不能存储有捕获的 lambda，不能存仿函数。
- **Callback3**：`std::function` 类型擦除，可存储任何可调用对象（函数指针、仿函数、lambda 包括有捕获的）。

```cpp
void register_cb(Callback1 cb);  // 只接收无捕获 lambda / 函数指针
void register_cb(Callback3 cb);  // 接收任何可调用对象

// 有捕获的 lambda 只能传给 Callback3
int state = 0;
register_cb([state](int x) { /* ... */ });  // 只能 Callback3
```

**HFT**：热路径用模板参数或 `Callback1/2`（可内联），非热路径用 `Callback3`（灵活但有开销）。
</details>

## 参考与延伸

- 上一节：[Item 44 头文件](item44-include-correct-headers.md)
- 下一节：[Item 46 成员函数 vs 算法](item46-member-vs-algorithm.md)
- [Effective Modern C++ Item 9：using 优于 typedef](../../../M1-modern-cpp/01-Effective-Modern-C++/ch03-moving-to-modern-cpp/README.md)
