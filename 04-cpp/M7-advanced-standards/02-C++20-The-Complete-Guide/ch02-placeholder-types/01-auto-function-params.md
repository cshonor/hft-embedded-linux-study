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

<details>
<summary>参考答案</summary>

1. 它是**简写函数模板**（abbreviated function template）：`void foo(auto x)` 等价于 `template <typename T> void foo(T x)`；`void foo(auto&& x)` 等价于 `template <typename T> void foo(T&& x)`（ forwarding reference 的推导规则不变）。
2. 本质：**它就是一个模板**，编译器为每个不同的实参类型各实例化一份。
因此定义必须对实例化点可见——**不能像普通函数那样只在 .cpp 里定义、在头文件里声明**（除非所有调用都在同一翻译单元，或做显式实例化）。通常放在头文件里，或使用 C++20 **模块**（`export` 后由模块接口提供）。
3. 把 concept 直接写在 `auto` 前面即可（约束占位类型）：
```cpp
void process(std::integral auto x);                 // 单参数
auto dot(std::floating_point auto a,
         std::floating_point auto b) { return a * b; }
```
其他等价写法：`template <std::integral T> void process(T x);`，或 `void process(auto x) requires std::integral<decltype(x)>;`。
注意写 `std::integral auto a, std::integral auto b` 时两个参数是**各自独立**约束的两个模板参数。
4. **不必相同**。每个 `auto` 参数都是**独立的**发明模板参数：
```cpp
auto add(auto a, auto b) { return a + b; }   // 等价 template<class T, class U> auto add(T, U)
add(1, 2.5);   // OK：T=int, U=double
```
所以它可以接受 `int` + `double`。若要求两个参数类型相同，必须写成显式模板：`template <class T> auto add(T a, T b);`，或用 `std::same_as` 之类的约束把第二个参数绑到第一个。
5. 几处差别：
   1. **同类型约束**：`auto add(auto, auto)` 的两个参数类型一定独立，无法表达"必须同类型"；显式模板 `template<class T> add(T, T)` 可以。
   2. **不能显式指定模板实参**：`foo<int>(42)` 对简写形式**不合法**（发明出来的模板参数没有名字/不在模板参数列表里），因而也无法对它做显式实例化与显式特化。
   3. **不能带其他模板参数**：需要非类型模板参数或额外类型参数时，只能回到完整 `template<...>` 语法。
   4. **可读性 vs 能力**：简写形式适合"纯泛型、无额外模板参数"的场景（日志、泛型比较）；需要精细控制（显式实参、偏特化、SFINAE 交互）时用完整模板。

</details>
