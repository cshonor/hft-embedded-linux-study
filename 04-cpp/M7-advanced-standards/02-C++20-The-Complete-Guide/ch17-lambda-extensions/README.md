# 第 17 章 Lambda 表达式扩展

**Lambda Extensions**

## 本章讲什么

C++20 给 lambda 的增量改进：模板 lambda、捕获结构化绑定、`[=, this]` 弃用警告、无状态 lambda 可默认构造/可赋值。

## 要点

### 模板 lambda

```cpp
// C++20：lambda 可有模板参数
auto f = []<typename T>(const std::vector<T>& v) {
    return v.size();   // T 在容器类型里，模板参数显式声明
};

std::vector<int> vi;
f(vi);   // T = int

// C++17 只能用 auto，但拿不到容器元素类型
auto g = [](const auto& v) { return v.size(); };  // 拿不到元素类型
```

模板 lambda 让你显式命名类型参数，用于需要元素类型的场景（如 `vector<T>` 的 `T`）。

```cpp
// 实用：约束容器元素类型
auto sum = []<std::integral T>(const std::vector<T>& v) {
    T s = 0;
    for (T x : v) s += x;
    return s;
};
```

### 捕获结构化绑定（修复）

```cpp
// C++17：结构化绑定变量不能被 lambda 捕获（编译错）
auto [x, y] = get_pair();
auto f = [x] { return x; };   // C++17 部分编译器错，C++20 修复

// C++20：OK
auto [x, y] = get_pair();
auto f = [x, y] { return x + y; };
```

C++17 的结构化绑定捕获在某些编译器有 bug，C++20 标准化修复。

### `[=, this]` 弃用

```cpp
class C {
    int x;
    auto make() {
        // C++20：[=] 隐式捕获 this 弃用警告
        return [=] { return x; };   // C++20 警告

        // 明确写
        return [this] { return x; };          // 推荐
        return [=, this] { return x; };       // C++20 显式
    }
};
```

C++20 起 `[=]` 隐式捕获 `this` 被弃用（易错），要显式写 `[this]` 或 `[*this]`（值捕获）。

### 无状态 lambda 的改进

```cpp
// C++20：无状态 lambda（不捕获）可默认构造、可赋值
auto f = [](int x) { return x; };
decltype(f) g;      // C++20 默认构造
g = f;              // C++20 赋值

// 用途：函数指针容器
std::vector<decltype(f)> fs;
fs.push_back(f);
```

C++17 之前无状态 lambda 不能默认构造/赋值，只能用函数指针。C++20 让 lambda 类型更"像类"。

### `consteval` lambda

```cpp
// C++20：consteval lambda（必须编译期调用）
auto sq = [](int x) consteval { return x * x; };
constexpr int v = sq(5);   // OK
// int y = sq(rand());     // 错误：必须编译期
```

## HFT 关联

- **模板 lambda 约束容器**：`[]<arithmetic T>(vector<T>&)` 约束只处理数值容器，编译期捕获类型错误。
- **`[this]` 显式捕获**：策略类成员函数的 lambda 明确写 `[this]`，C++20 不再容忍 `[=]` 隐式捕获 this 的歧义。
- **`[*this]` 值捕获**：异步回调用 `[*this]` 拷贝策略对象，避免 this 悬垂（第 17 章复习）。
- **无状态 lambda 做比较器**：`std::set<T, decltype(cmp)>` 用无状态 lambda 类型做比较器，C++20 可默认构造。
- **consteval lambda 编译期计算**：参数表/查找表用 consteval lambda 编译期生成。

## 自测题

1. 模板 lambda 相比 C++17 的 `auto` lambda 有什么优势？
2. C++20 为什么弃用 `[=]` 隐式捕获 this？推荐怎么写？
3. 无状态 lambda 在 C++20 有什么新能力？有什么用？
4. C++20 修复了结构化绑定捕获的什么问题？
5. HFT 用模板 lambda 约束数值容器怎么写？

## 代码自测

### Q1: 模板 lambda
```cpp
// C++20: lambda 可以有模板参数
auto deref = []<typename T>(T* p) { return *p; };

int x = 42;
deref(&x);  // OK，T = int

// 也可以约束
auto add = []<std::integral T>(T a, T b) { return a + b; };
add(1, 2);     // OK
// add(1.0, 2.0);  // 编译错误：double 不满足 integral
```
> C++20 lambda 还有哪些扩展？

<details>
<summary>答案与复习指引</summary>

C++20 lambda 扩展：
1. **模板参数**：`[]<typename T>(T* p) { return *p; }`
2. **concept 约束**：`[]<std::integral T>(T a, T b) { ... }`
3. **可默认构造/可赋值**：无捕获的 lambda 可默认构造（C++20 前不能），可赋值
4. **pack expansion in lambda**：`[](auto... args) { return (args + ...); }`（C++17 已有，C++20 完善）

**模板 lambda 的价值**：
```cpp
// C++17: 需要 auto，但不能对指针特化
auto deref17 = [](auto* p) { return *p; };

// C++20: 可以写模板参数，更精确
auto deref20 = []<typename T>(T* p) -> T& { return *p; };
// 可以返回引用，auto 版本会丢失引用
```

**复习：** → [Lambda 扩展](./README.md)
</details>
