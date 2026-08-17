# 第 6 章 Lambda 扩展

**Lambda Extensions**

## 本章讲什么

C++17 给 lambda 加了几个实用增强：`constexpr` lambda、捕获 `*this`（值捕获当前对象）、泛型 lambda 的改进。

## 要点

### `constexpr` lambda

```cpp
// C++17：lambda 可声明为 constexpr
constexpr auto square = [](int x) { return x * x; };
static_assert(square(5) == 25);   // 编译期调用

// 隐式 constexpr：如果 lambda 体满足 constexpr 条件，自动是 constexpr
auto add = [](int a, int b) { return a + b; };  // 隐式 constexpr
constexpr int x = add(1, 2);   // OK
```

### 捕获 `*this`（值捕获当前对象）

```cpp
class Worker {
    int data;
public:
    auto make_task() {
        // C++14：[this] 捕获指针，对象析构后悬垂
        // C++17：[*this] 值拷贝当前对象，安全
        return [*this]() { return data; };
    }
};
```

C++14 只能 `[this]`（捕获指针），对象生命周期结束后 lambda 调用就 use-after-free。C++17 的 `[*this]` 拷贝整个对象，异步回调安全。

### 泛型 lambda 改进

```cpp
// C++14：泛型 lambda 用 auto
auto f = [](auto x) { return x; };

// C++17：可用于更复杂场景
auto cmp = [](const auto& a, const auto& b) { return a < b; };
std::sort(v.begin(), v.end(), cmp);   // 异质比较
```

### 捕获初始化的更多用法

```cpp
// 移动捕获（C++14 引入，C++17 更常用）
auto p = std::make_unique<Obj>();
auto task = [p = std::move(p)]() { p->work(); };   // 移动捕获 unique_ptr

// 捕获表达式结果
auto lambda = [size = vec.size()](int i) { return i < size; };
```

## HFT 关联

- **`constexpr` lambda 做编译期计算**：参数表/阈值表的生成用 constexpr lambda 在编译期算好，运行期零开销。
- **`[*this]` 用于异步回调**：策略对象注册定时器回调用 `[*this]`，即使策略对象析构，回调持有的拷贝仍安全。但注意拷贝开销——大对象慎用。
- **移动捕获智能指针**：把 `unique_ptr<Session>` 移动捕获进 lambda，避免裸 this 悬垂。
- **泛型 lambda 替代模板函数**：热路径的少量比较器用泛型 lambda，内联友好。
- **捕获 `size = vec.size()`**：避免 lambda 内重复调用 `.size()`（可能有原子操作），捕获时算一次。

## 自测题

1. `constexpr` lambda 的意义是什么？隐式 constexpr 的条件是什么？
2. `[*this]` 和 `[this]` 的区别？解决了什么安全问题？
3. 移动捕获 `unique_ptr` 的写法是什么？
4. 捕获 `[size = vec.size()]` 比在 lambda 内调 `vec.size()` 好在哪？
5. HFT 异步回调为什么用 `[*this]` 而非 `[this]`？有什么代价？

## 代码自测

### Q1: constexpr lambda
```cpp
// C++17: lambda 可以是 constexpr
auto square = [](int x) constexpr { return x * x; };
static_assert(square(5) == 25);  // 编译期求值
```
> constexpr lambda 意味着什么？什么条件下 lambda 自动成为 constexpr？

<details>
<summary>答案与复习指引</summary>

**constexpr lambda**：lambda 的 `operator()` 隐式为 `constexpr`（如果函数体满足 constexpr 要求）。C++17 起可以显式写 `constexpr`。

**条件**：lambda 体中不能有非常量表达式（如动态分配、虚函数调用、throw）。

**用途**：
```cpp
// 编译期排序
constexpr int arr[] = {3, 1, 4, 1, 5};
std::sort(arr, arr+5, [](int a, int b) constexpr { return a < b; });
// C++20 才支持 constexpr sort，C++17 的 constexpr lambda 可用于自定义编译期计算
```

**复习：** → [constexpr lambda](./README.md)
</details>

### Q2: 捕获 *this
```cpp
struct Processor {
    int data = 42;
    auto getCallback() {
        // C++14: [this] 捕获指针，对象销毁后悬垂
        // C++17: [*this] 捕获拷贝，安全
        return [*this]() { return data; };
    }
};
```
> `[this]` 和 `[*this]` 的区别？为什么 C++17 要引入 `*this` 捕获？

<details>
<summary>答案与复习指引</summary>

- **`[this]`**：捕获 `this` 指针。lambda 通过指针访问对象成员。如果对象先于 lambda 销毁 → **悬垂指针**。
- **`[*this]`**（C++17）：捕获对象的**拷贝**。lambda 内有自己的对象副本，安全。但拷贝有开销（大对象慎用）。

**场景**：异步回调返回后原对象可能已销毁 → 用 `[*this]`。如果原对象保证存活 → `[this]` 更高效。

**复习：** → [捕获 *this](./README.md)
</details>
