# auto 函数参数

## 基本用法

```cpp
// C++20：auto 参数 = 简写模板
void print(const auto& x) {
    std::cout << x;
}

// 等价于：
template <typename T>
void print(const T& x) {
    std::cout << x;
}

// 多个 auto
auto add(auto a, auto b) { return a + b; }
// 等价于：
template <typename T, typename U>
auto add(T a, U b) { return a + b; }
```

## 本质是模板

```cpp
// 这两个完全等价
void foo(auto x, auto y);
template <typename T, typename U>
void foo(T x, U y);

// auto 参数的函数是模板——每个不同类型组合生成一份实例
foo(1, 2);      // foo<int, int>
foo(1, 2.0);    // foo<int, double>
foo("a", 'b');  // foo<const char*, char>
```

## 配合 Concepts 约束

```cpp
// C++20：auto + concept 约束
void process(std::integral auto x) {
    // 只有整数类型能调用
}

process(42);     // ✅
process(3.14);   // ❌ double 不满足 integral

// 多参数约束
auto dot(std::floating_point auto a, std::floating_point auto b) {
    return a * b;
}
```

## 与模板的细微区别

```cpp
// auto 参数不能在 .cpp 中定义（模板必须在头文件）
// 但可以用 inline 或模块

// auto 参数的显式实例化
template void foo<int>(int);  // 模板版
// foo<int>(42);  // auto 版不能这样显式实例化

// SFINAE 友好度不同
// auto 版在某些 SFINAE 场景行为略有差异
```

## 实际应用

```cpp
// 1. 泛型打印/日志
void log(auto&&... args) {
    (std::cout << ... << args) << '\n';
}

// 2. 泛型比较
bool less_than(const auto& a, const auto& b) {
    return a < b;
}

// 3. 配合 Concepts 的策略选择
void on_data(std::movable auto&& data) {
    // data 必须可移动
    process(std::forward<decltype(data)>(data));
}
```

## 自测题

1. `void foo(auto x)` 等价于什么模板写法？
2. `auto` 参数的函数本质是什么？能否在 .cpp 中定义？
3. 如何给 `auto` 参数加 concept 约束？
4. `auto add(auto a, auto b)` 的 `a` 和 `b` 类型必须相同吗？
5. `auto` 参数和模板参数有什么细微区别？
