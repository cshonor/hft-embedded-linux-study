# A.2 lambda 表达式

> 附录 A · 上一节：[A.1 右值引用与移动语义](01-rvalue-move.md) · 下一节：[A.3 智能指针](03-smart-ptr.md)

## 这节讲什么

lambda 是 C++11 最重要的特性之一——匿名函数，可以捕获上下文变量。本节讲 lambda 的语法、捕获方式（值/引用/默认）、以及它在并发中的核心作用（`std::thread`、`std::async` 的回调）。

---

## 核心规则（代码+表格）

### lambda 语法

```cpp
auto f = capture mutable -> return_type { body };
//       ^捕获    ^参数   ^可改写 ^返回类型      ^函数体
```

### 捕获方式

```cpp
int x = 10, y = 20;

// 值捕获：拷贝 x 的当前值
auto f1 = [x](int a) { return a + x; };  // x=10 被拷贝

// 引用捕获：共享 x（x 变化会影响 lambda）
auto f2 = [&x](int a) { x = a; };  // 修改外部 x

// 默认值捕获：所有用到的变量都值捕获
auto f3 = [=](int a) { return a + x + y; };

// 默认引用捕获：所有用到的变量都引用捕获
auto f4 = [&](int a) { x = a; y = a; };

// 混合：默认值 + 特定引用
auto f5 = [=, &x](int a) { x = a; return y; };

// C++14 初始化捕获（移动捕获）
auto p = std::make_unique<int>(42);
auto f6 = [p = std::move(p)] { return *p; };  // 移动捕获 unique_ptr

// C++14 泛型 lambda
auto f7 = [](auto a, auto b) { return a + b; };  // 可接受任意类型
```

### 捕获方式对比

| 捕获 | 语法 | 何时拷贝 | 并发安全性 |
|------|------|----------|-----------|
| 值捕获 | `[x]` | lambda 创建时 | 安全（独立副本） |
| 引用捕获 | `[&x]` | 不拷贝，共享 | 危险（需同步） |
| 默认值 | `[=]` | 创建时 | 安全 |
| 默认引用 | `[&]` | 不拷贝 | 危险 |
| 移动捕获 | `[p=move(p)]` | 创建时移动 | 安全（C++14） |

### lambda 在并发中的核心作用

```cpp
// 1. std::thread 的回调
int data = 42;
std::thread t([data]{ std::cout << data; });  // 值捕获，安全
t.join();

// 2. std::async 的回调
auto fut = std::async(std::launch::async, [data]{ return data * 2; });

// 3. 线程池任务
pool.submit([this]{ process(); });  // 捕获 this

// 4. 条件变量谓词
bool ready = false;
cv.wait(lk, [&ready]{ return ready; });  // 引用捕获
```

### `[this]` 的陷阱

```cpp
class Worker {
    int data;
public:
    void start() {
        // 捕获 this → 如果 Worker 析构，this 悬空
        std::thread t([this]{ std::cout << data; });
        t.detach();  // 危险！
    }
};
// Worker 析构后，t 可能还在执行 → use-after-free

// C++20 更安全的写法：捕获 *this（拷贝）
void start_safe() {
    std::thread t([this, *this]{ /* 拷贝了整个对象 */ });
    // 或用 shared_ptr（见 A.3）
}
```

---

## 新手要点（和 C 的区别）

- **C 用函数指针 + `void*` 参数**：C 的回调是 `void callback(void* arg)`——类型不安全、要手动传参数。C++ 的 lambda 可以捕获上下文，无需额外参数——这是巨大进步。
- **"捕获"是 C 程序员的新概念**：C 的函数指针不捕获任何东西。C++ 的 lambda 可以"记住"创建时的变量——值捕获拷贝、引用捕获共享。C 程序员要理解这个区别。
- **引用捕获在并发中危险**：C 程序员可能习惯传指针（类似引用）——但在多线程中，引用捕获的变量可能被多线程同时访问。值捕获更安全（独立副本）。
- **`mutable` 关键字**：值捕获的变量默认是 `const`，不能在 lambda 内修改。加 `mutable` 可以修改（但修改的是拷贝，不影响原变量）。C 程序员可能不理解这个限制。

---

## HFT 关联

- **lambda 是 HFT 并发代码的标准写法**：`std::thread t([&]{ process(); });` 是 HFT 系统中线程创建的标配——比 C 的函数指针+参数简洁得多。
- **值捕获避免共享**：HFT 中如果线程需要独立的数据副本，用值捕获 `[=]`——每线程拷贝一份，无竞争。
- **`[this]` 在 HFT 回调中的风险**：HFT 的定时器回调、网络回调如果捕获 `this`，对象可能已析构。用 `shared_from_this` 或值捕获 `[*this]`（C++17）。
- **lambda 的内联优化**：lambda 是编译期生成的，编译器可以完美内联——比 C 的函数指针（通过指针调用，难内联）性能更好。HFT 热路径的回调用 lambda。

---

## 自测题

1. 值捕获和引用捕获有什么区别？分别在什么时候拷贝？
2. `[=]` 和 `[&]` 各捕获什么？在并发中哪个更安全？
3. C++14 的初始化捕获（`[p = std::move(p)]`）解决了什么问题？
4. 为什么 `[this]` 在 detach 的线程中危险？
5. lambda 相比 C 的函数指针有什么性能优势？

---

## 参考与延伸

- 下一节：[A.3 智能指针](03-smart-ptr.md)
- 上一节：[A.1 右值引用与移动语义](01-rvalue-move.md)
- 回到：[附录 A](README.md)
