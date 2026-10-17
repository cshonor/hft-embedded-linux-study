# 第 9 章 类模板参数推导 CTAD

**Class Template Argument Deduction**

## 本章讲什么

C++17 之前写 `std::pair<int, std::string> p(1, "hi")` 要手写模板参数。C++17 引入 CTAD（Class Template Argument Deduction），让编译器从构造函数参数推导模板参数，像 `auto` 之于函数那样应用于类模板。

## 要点

### 基本用法

```cpp
// C++14
std::pair<int, std::string> p(1, "hi");
std::vector<int> v = {1, 2, 3};
std::lock_guard<std::mutex> lg(m);

// C++17 CTAD
std::pair p(1, "hi");        // 推导 pair<int, std::string>
std::vector v = {1, 2, 3};   // 推导 vector<int>
std::lock_guard lg(m);       // 推导 lock_guard<std::mutex>
std::atomic a{0};            // 推导 atomic<int>
```

### 推导指引（deduction guide）

编译器默认从构造函数签名推导。你可以写**推导指引**自定义推导规则：

```cpp
template <typename T>
struct Container {
    Container(T val) {}
};

// 推导指引：从 int 推导 Container<long>
Container(int) -> Container<long>;

Container c(42);   // Container<long>，而非 Container<int>
```

### 标准库的 CTAD

| 类型 | 推导示例 |
|------|----------|
| `std::pair` | `pair p(1, 2.0)` → `pair<int, double>` |
| `std::tuple` | `tuple t(1, "hi", 3.0)` → `tuple<int, const char*, double>` |
| `std::vector` | `vector v = {1,2,3}` → `vector<int>` |
| `std::array` | `array a{1,2,3}` → `array<int, 3>`（C++17 有 array 推导指引） |
| `std::lock_guard` | `lock_guard lg(m)` → `lock_guard<mutex>` |
| `std::atomic` | `atomic a{0}` → `atomic<int>` |
| `std::optional` | `optional o = 42` → `optional<int>` |

### CTAD 的限制

- **没有构造函数的类**：聚合类型也可 CTAD（C++17 聚合推导）。
- **默认模板参数**：推导时不用默认模板参数（除非有推导指引）。
- **C++17 无部分推导**：`std::vector<T, Alloc>` 写 `vector<int>` 推不出 `Alloc`，要全写或靠指引。C++20 改进。

## HFT 关联

- **简化容器声明**：`vector ticks = {Tick{...}, Tick{...}}` 不用写 `<Tick>`，代码更干净。
- **`lock_guard lg(m)` 省类型**：多 mutex 类型场景 `scoped_lock lk(m1, m2)` 自动推导，少出错。
- **`atomic` 简写**：`atomic seq{0}` 比 `atomic<uint64_t> seq{0}` 简洁，热路径计数器声明清爽。
- **推导指引自定义类型**：为 `RingBuffer<T, N>` 写 `RingBuffer() -> RingBuffer<T, 1024>` 提供默认容量。
- **C++20 的 CTAD 改进**：C++20 加 Concepts 约束 CTAD，推导更安全——C++17 是过渡，但要掌握基础。

## 自测题

1. CTAD 解决了什么麻烦？C++14 之前怎么写？
2. 推导指引（deduction guide）的作用是什么？语法是什么？
3. `std::lock_guard lg(m)` 推导出什么类型？
4. CTAD 在 C++17 有什么限制？默认模板参数参与推导吗？
5. HFT 用 CTAD 声明 `atomic` 计数器有什么好处？

## 代码自测

### Q1: CTAD（类模板参数推导）
```cpp
// C++14
std::pair<int, double> p(1, 2.0);
std::vector<int> v = {1, 2, 3};

// C++17
std::pair p(1, 2.0);     // 推导 pair<int, double>
std::vector v = {1, 2, 3}; // 推导 vector<int>
std::lock_guard lk(m);   // 推导 lock_guard<mutex>
```
> CTAD 何时工作？什么时候需要推导指引？

<details>
<summary>答案与复习指引</summary>

**CTAD 何时工作**：当构造对象时不写模板参数，编译器根据构造函数参数类型推导。

**需要推导指引**：当默认推导规则不够时。例如：
```cpp
// std::pair 的推导指引（简化）
template<typename T, typename U>
pair(T, U) -> pair<T, U>;

// 自定义推导指引
template<typename T>
struct Container {
    Container(T* p, size_t n) {}
};
// 默认推导 Container<int*>，但可能想要 Container<int>
Container(T* p, size_t n) -> Container<typename std::remove_pointer<T>::type>;
```

**注意**：CTAD 不适用于函数返回类型（需显式写 `std::pair<int,double> f()`）。

**复习：** → [CTAD](./README.md)
</details>
