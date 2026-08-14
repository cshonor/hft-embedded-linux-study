# 9.1 类模板参数推导（CTAD）基本概念

> 第 9 章 类模板参数推导 CTAD · 下一节：[9.2 推导指南与常见陷阱](02-推导指南与常见陷阱.md)

## 这节讲什么

C++17 之前，声明模板类对象必须写全模板参数：`std::pair<int, double> p(1, 2.0);`。C++17 让编译器从构造函数参数自动推导模板参数：`std::pair p(1, 2.0);`。这叫 CTAD（Class Template Argument Deduction）。

## 为什么要学这个（先建立直觉）

C 程序员用宏或 typedef 来简化类型名。C++ 模板提供了泛型，但写起来啰嗦：

```cpp
// C++14：必须写全模板参数
std::pair<int, double> p(1, 2.0);
std::vector<int> v = {1, 2, 3};
std::lock_guard<std::mutex> lg(mtx);
std::atomic<int> counter(0);

// C++17：编译器从参数推导
std::pair p(1, 2.0);           // pair<int, double>
std::vector v = {1, 2, 3};     // vector<int>
std::lock_guard lg(mtx);       // lock_guard<std::mutex>
std::atomic counter(0);        // atomic<int>
```

## CTAD 的工作原理

编译器尝试用构造函数参数推导模板参数，类似于函数模板推导：

```cpp
// pair 的构造函数：template<typename A, typename B> pair(const A&, const B&);
// std::pair p(1, 2.0);
// 编译器推导：A=int, B=double → pair<int, double>
```

### 标准库中广泛可用

```cpp
std::tuple t(1, 2.0, "hello");    // tuple<int, double, const char*>
std::optional opt(42);             // optional<int>
std::variant v(1);                 // variant<int>
std::array arr{1, 2, 3, 4};       // array<int, 4>
std::function f = [](int x) { return x * 2; };  // function<int(int)>
```

### 拷贝推导

```cpp
std::vector<int> v1 = {1, 2, 3};
std::vector v2 = v1;  // CTAD: vector<int>（从 v1 的类型推导）
```

## 零开销

CTAD 是编译期推导，运行时零开销。推导结果和手写完全一样：

```cpp
std::pair p(1, 2.0);
// 等价于
std::pair<int, double> p(1, 2.0);
// 生成的代码完全相同
```

## HFT 关联

```cpp
// 简化热路径代码
auto result = std::pair(status, latency);  // 不用写 pair<int, uint64_t>

// 原子变量
std::atomic count(0);              // atomic<int>
std::atomic flag(false);           // atomic<bool>

// lock_guard
std::lock_guard lg(mtx);           // 不用写 mutex 类型

// SPSC 队列模板
RingBuffer ring(65536);            // CTAD 推导元素类型
```

## 小结

| C++14 | C++17 |
|-------|-------|
| `std::pair<int, double> p(1, 2.0)` | `std::pair p(1, 2.0)` |
| `std::lock_guard<std::mutex> lg(m)` | `std::lock_guard lg(m)` |
| `std::atomic<int> c(0)` | `std::atomic c(0)` |
| 必须写全模板参数 | CTAD 自动推导 |

---

← [本章导读](./README.md) · [下一节 →](02-推导指南与常见陷阱.md)
