# A.5 可变参数模板

> 附录 A · 上一节：[A.4 auto 与 decltype](04-auto-decltype.md) · 下一节：[A.6 constexpr](06-constexpr.md)

## 这节讲什么

可变参数模板（variadic template）让模板接受任意数量、任意类型的参数。本节讲参数包（parameter pack）的展开、递归模式、折叠表达式（C++17）、以及在并发中的应用（`std::thread` 接受任意参数）。

---

## 核心规则（代码+表格）

### 参数包基础

```cpp
// typename... 声明参数包
template <typename... Args>
void print(Args... args) {
    // args 是参数包，可以包含 0 到 N 个参数
}

print();           // Args = {}，0 个参数
print(1);          // Args = {int}
print(1, 2.0, "x"); // Args = {int, double, const char*}
```

### 递归展开（C++11）

```cpp
// 递归终止
void print() {}

// 递归：取第一个参数，递归处理剩余
template <typename T, typename... Rest>
void print(T first, Rest... rest) {
    std::cout << first << " ";
    print(rest...);  // 递归
}

print(1, 2.0, "hello");
// → print(1, 2.0, "hello")
// → cout << 1; print(2.0, "hello")
// → cout << 2.0; print("hello")
// → cout << "hello"; print()
```

### 折叠表达式（C++17）

```cpp
// C++17 折叠表达式：一行展开参数包
template <typename... Args>
auto sum(Args... args) {
    return (args + ...);  // 二元右折叠：arg1 + (arg2 + (arg3 + ...))
}

template <typename... Args>
void print_all(Args... args) {
    ((std::cout << args << " "), ...);  // 逗号折叠
}

sum(1, 2, 3, 4);  // 10
print_all(1, 2.0, "x");  // 1 2.0 x
```

### `sizeof...(args)` 查询参数数量

```cpp
template <typename... Args>
void info(Args... args) {
    std::cout << "num args: " << sizeof...(args) << "\n";
    std::cout << "num types: " << sizeof...(Args) << "\n";
}
info(1, 2, 3);  // num args: 3, num types: 3
```

### `std::thread` 的可变参数应用

```cpp
// std::thread 内部用可变参数模板转发参数
template <typename F, typename... Args>
explicit thread(F&& f, Args&&... args);

// 可以传任意参数给线程函数
void work(int a, double b, const std::string& c) { ... }

std::thread t1(work, 1, 2.0, "hello");  // 3 个参数
std::thread t2(work, 10, 3.14, std::string("world"));

// 注意：参数默认按值拷贝（decay）
std::string s = "test";
std::thread t3([&s]{ use(s); });     // 引用捕获（手动）
std::thread t4(some_func, s);        // 值拷贝（自动）
std::thread t5(some_func, std::ref(s));  // 引用传递（显式）
```

### `std::forward` 完美转发

```cpp
// 可变参数 + 完美转发：保留参数的左右值属性
template <typename... Args>
void wrapper(Args&&... args) {
    target(std::forward<Args>(args)...);  // 逐个完美转发
}

// forward 确保左值传左值、右值传右值
```

---

## 新手要点（和 C 的区别）

- **C 用 `va_list`/`va_start`/`va_arg` 处理可变参数**：C 的 `printf` 就是可变参数函数。但 C 的可变参数**不安全**——无类型检查、无参数数量。C++ 的可变参数模板是编译期类型安全的——每个参数的类型在编译期已知。
- **C 的 `va_arg` 要手动指定类型**：`va_arg(ap, int)` 如果类型错就 UB。C++ 的可变参数模板自动推导类型——这是巨大进步。
- **递归展开是 C 程序员陌生的模式**：C 程序员可能不理解"为什么要递归展开参数包"——因为 C++11 没有直接的展开语法，只能递归。C++17 的折叠表达式简化了这一点。
- **`std::forward` 是 C++ 特有**：C 没有左右值的概念，也不需要转发。C++ 的 `std::forward` 配合万能引用实现"完美转发"——这是 C++ 高级模板编程的基础。

---

## HFT 关联

- **`std::thread` 的可变参数让 HFT 线程创建简洁**：`std::thread t(strategy, tick_data, config, risk_limit);`——任意参数直接传递，无需封装成 struct。
- **`make_unique`/`make_shared` 用可变参数**：`std::make_unique<Order>(symbol, price, quantity);`——完美转发参数到构造函数。HFT 中构造对象常用。
- **线程池的 `submit` 用可变参数**：`pool.submit(task_func, arg1, arg2);`——HFT 线程池的通用接口靠可变参数模板实现。
- **折叠表达式简化 HFT 工具**：如批量设置多个 atomic 标志、批量 join 多个线程——C++17 折叠表达式一行搞定。

---

## 自测题

1. 可变参数模板和 C 的 `va_list` 有什么本质区别？
2. C++11 如何展开参数包？C++17 的折叠表达式有什么改进？
3. `sizeof...(args)` 返回什么？
4. `std::thread(func, arg1, arg2)` 的参数是如何传递的？默认是值还是引用？
5. `std::forward` 在可变参数模板中起什么作用？

---

## 参考与延伸

- 下一节：[A.6 constexpr](06-constexpr.md)
- 上一节：[A.4 auto 与 decltype](04-auto-decltype.md)
- 回到：[附录 A](README.md)
