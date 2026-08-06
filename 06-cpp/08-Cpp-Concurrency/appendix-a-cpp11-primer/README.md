# 附录 A C++11 精要

**A Brief Introduction to C++11 Language Features**

## 本附录讲什么

并发编程大量依赖 C++11 引入的语言特性。本附录快速过一遍右值引用/移动语义、lambda、智能指针、`auto`/`decltype`、可变参数模板等——这些是理解本书代码的前提。

## 要点

### 右值引用与移动语义

```cpp
std::vector<int> make() { return std::vector<int>(1000, 1); }
std::vector<int> v = make();   // C++11：移动而非拷贝，零深拷贝
```

- **左值**：有名字、可取地址（`v`、`arr[i]`）。
- **右值**：临时的、即将销毁的（`make()` 的返回值、`x+y`）。
- **右值引用 `T&&`**：绑定到右值，表示"我可以偷你的资源"。
- **移动构造/赋值**：偷源对象的资源指针，源对象置空——O(1) 而非 O(n)。

`std::move(x)` 只是把它转成右值引用，**不移动任何东西**——真正的移动由移动构造函数完成。

### lambda 表达式

```cpp
int x = 10;
auto f = [x](int a) { return a + x; };   // 值捕获 x
auto g = [&x](int a) { x = a; };          // 引用捕获 x
auto h = [=](int a) { return a; };        // 值捕获所有用到的
auto i = [&](int a) { return a; };        // 引用捕获所有用到的
```

- `[capture](params) -> ret { body }`。
- 值捕获在 lambda **创建时**拷贝，引用捕获共享原变量。
- C++14 起支持**初始化捕获** `[p = std::move(ptr)]`，可以移动捕获。
- C++14 起支持**泛型 lambda** `[](auto x){}`。

并发中 lambda 极常用——`std::thread([]{ ... })`、`std::async(std::launch::async, [&]{ ... })`。

### 智能指针

| 类型 | 语义 | 代价 |
|------|------|------|
| `unique_ptr<T>` | 独占所有权 | 零开销（和裸指针一样） |
| `shared_ptr<T>` | 共享所有权，引用计数 | 原子计数，有开销 |
| `weak_ptr<T>` | 观察者，不增计数 | 破环 |

```cpp
std::unique_ptr<Obj> p = std::make_unique<Obj>();   // C++14
std::shared_ptr<Obj> s = std::make_shared<Obj>();   // 一次分配（对象+控制块）
```

并发要点：
- `shared_ptr` 的**引用计数是原子的**（`shared_ptr` 对象本身的拷贝/析构线程安全）。
- 但**指向的对象不是**——多线程改同一个 `shared_ptr` 指向的对象仍需同步。
- `shared_ptr` 的**赋值（换指向）不是线程安全**——多个线程写同一个 `shared_ptr` 变量要 `atomic<shared_ptr>`（C++20）或锁。

### `auto` 与 `decltype`

```cpp
auto x = 42;              // int
auto& y = v[0];           // int&
decltype(v[0]) z = v[0];  // int&（decltype 保留引用性）
decltype(auto) w = v[0];  // C++14，同上
```

并发中 `auto` 让 `std::async`、`std::future` 的复杂返回类型易写。

### 可变参数模板

```cpp
template <typename... Args>
void print(Args... args) { (std::cout << ... << args) << '\n'; }  // C++17 折叠表达式

template <typename... Args>
auto make_thread(Args&&... args) {
    return std::thread(std::forward<Args>(args)...);   // 完美转发
}
```

`thread`、`async`、`promise::set_value` 都用可变参数模板 + 完美转发接受任意参数。

### `constexpr`

```cpp
constexpr int factorial(int n) { return n <= 1 ? 1 : n * factorial(n - 1); }
constexpr int x = factorial(5);   // 编译期求值，120
```

并发中 `constexpr` 用于编译期常量（队列容量、缓冲大小），避免运行期初始化竞争。

## HFT 关联

- **移动语义零拷贝**：行情对象在流水线线程间用 `std::move` 传递，避免深拷贝。
- **`unique_ptr` 零开销**：HFT 热路径用 `unique_ptr` 管理资源，析构确定无泄漏。
- **`shared_ptr` 慎用于热路径**：原子计数有 cache 同步开销，热路径用 `unique_ptr` 或栈对象。
- **lambda 捕获策略上下文**：策略线程的 lambda 捕获策略参数，简洁高效。
- **`constexpr` 常量**：队列大小、缓冲容量用 `constexpr`，编译期确定无运行初始化。
- **完美转发**：任务封装层用 `forward<Args>` 转发参数到 `thread`/`async`。

## 自测题

1. `std::move(x)` 做了什么？为什么不移动任何东西？
2. lambda 的值捕获和引用捕获有什么区别？什么时候用初始化捕获？
3. `shared_ptr` 的引用计数是原子的，为什么还说它不是完全线程安全？
4. 可变参数模板 + 完美转发在 `std::thread` 中如何工作？
5. HFT 热路径为什么用 `unique_ptr` 而非 `shared_ptr`？
