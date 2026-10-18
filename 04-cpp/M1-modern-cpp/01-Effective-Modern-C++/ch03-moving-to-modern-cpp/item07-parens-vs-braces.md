# Item 7：区别 () 和 {} 创建对象

> 第 3 章 移步现代 C++ · Item 7 · 上一节：[Item 6 auto 看不出来的显式类型初始化器](../ch02-auto/item06-explicitly-typed-initializer.md)

## 为什么要学这个（先建立直觉）

C 里初始化很简单——`int x = 42;`、`int arr[] = {1, 2, 3};`、`struct Point p = {3, 4};`，要么 `=` 要么 `{}`，没有第三种。C++ 有了类和构造函数重载之后，初始化出现了 `()` 写法，C++11 又给了 `{}` 一个"万能初始化"的名头：数组、容器、内置类型、成员初始化列表全都能用。听起来很美——写代码时闭着眼用 `{}` 就行了？

不行。`std::vector<int> v(10, 20)` 和 `std::vector<int> v{10, 20}` 一个是"10 个 20"、一个是"两个元素 10 和 20"，语义天差地别，而两行代码只差一对括号。这类 bug 编译不报错、运行不崩溃，只是**容器里的内容不对**——在订单簿代码里就是"预分配 1000 档"变成"两档，值是 1000 和 0"这种隐蔽事故。

本节的全部内容，其实就是回答一个问题：**同一个参数列表，什么时候该用 `()`，什么时候该用 `{}`，以及选错了会发生什么。**

---

## 这节讲什么

1. `()` 与 `{}` 的三条系统性差异（窄化防护、最恼人解析、`initializer_list` 优先）；
2. `initializer_list` 构造函数的"劫持"行为，以及它唯一的退让条件；
3. 为什么模板代码里 `()` 和 `{}` 语义不可互换，作者只能把选择权留给用户；
4. 全部论断附本机实测（clang 23.1.0，arm64-apple-darwin，libc++，`-std=c++17 -Wall`）。

---

## () vs {} 总览对比

| 特性 | `()` | `{}` |
|------|------|------|
| 窄化转换 | 允许（`int(3.14)` 得 3） | **禁止**（编译失败，报错原文见下） |
| 最恼人解析 | 会中招（`Widget w();` 被解析成函数声明） | 免疫 |
| `initializer_list` 构造函数 | 不参与优先规则 | **优先匹配**，除非完全不可行 |
| 空初始化 `T x()` / `T x{}` | 函数声明（不创建对象！） | 默认构造 / 值初始化 |
| 适用范围 | 只能调构造函数 | 几乎一切：数组、容器、聚合体、成员初始化、返回值 |

一句话：`{}` 更安全（防窄化、防解析歧义），但对有 `initializer_list` 构造函数的类，`{}` 会改变重载决议结果。

---

## initializer_list 劫持：本节的核心

### vector 的经典陷阱（实测）

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v1(10, 20);   // 调 (count, value) 构造
    std::vector<int> v2{10, 20};   // 被 initializer_list 构造劫持
    std::vector<int> v3(10);       // 10 个 0
    std::vector<int> v4{10};       // 1 个元素 10
    std::vector<int> v5{};         // 空 vector（空 {} 走默认构造）

    std::cout << "v1.size()=" << v1.size() << " v1[0]=" << v1[0] << "\n";
    std::cout << "v2.size()=" << v2.size() << " v2[0]=" << v2[0] << " v2[1]=" << v2[1] << "\n";
    std::cout << "v3.size()=" << v3.size() << " v3[0]=" << v3[0] << "\n";
    std::cout << "v4.size()=" << v4.size() << " v4[0]=" << v4[0] << "\n";
    std::cout << "v5.size()=" << v5.size() << "\n";
}
```

本机实测 clang 23.1.0 输出：

```
v1.size()=10 v1[0]=20
v2.size()=2 v2[0]=10 v2[1]=20
v3.size()=10 v3[0]=0
v4.size()=1 v4[0]=10
v5.size()=0
```

`{10, 20}` 没有匹配"10 个 20"的构造函数，而是整体变成一个 `initializer_list<int>`，两个值原样入容器。注意空 `{}` 是特例：它不生成空 `initializer_list`，而是调默认构造。

### 劫持的优先级有多强：宁可类型转换也要走 init_list

```cpp
class Widget {
public:
    Widget(int i, bool b)      { std::cout << "ctor(int,bool)\n"; }
    Widget(int i, double d)    { std::cout << "ctor(int,double)\n"; }
    Widget(std::initializer_list<long double> il) {
        std::cout << "ctor(init_list), size=" << il.size() << "\n";
    }
};

int main() {
    Widget w1(10, true);    // ?
    Widget w2{10, true};    // ?
    Widget w3(10, 5.0);     // ?
    Widget w4{10, 5.0};     // ?
}
```

本机实测 clang 23.1.0 输出：

```
ctor(int,bool)
ctor(init_list), size=2
ctor(int,double)
ctor(init_list), size=2
```

`w2{10, true}` 明明有**精确匹配**的 `ctor(int, bool)`，编译器仍然选择把 `10` 和 `true` 转成 `long double` 塞进 `initializer_list`。**只要 `{}` 里的元素能以任何方式（除窄化外）转换成 init_list 的元素类型，init_list 构造函数就赢。**这就是 Meyers 说的"劫持"。

### 唯一的退让：元素类型完全转不动时

```cpp
std::vector<std::string> vs{10, "hi"};   // int 转不了 string
```

实测输出：`vs.size()=10 vs[0]=hi`。`initializer_list<std::string>` 走不通，编译器才退回普通重载决议，匹配 `(size_type, const string&)`。**所以 `{}` 的语义取决于元素类型能否转换——给类新增一个 `initializer_list` 构造函数，可能静默改变所有已有 `{}` 调用点的行为。**

---

## 窄化防护：{} 的安全红利

```cpp
double d = 3.14;
int x1(d);       // OK：静默截断为 3
int x2{3.14};    // 编译失败
int x3{d};       // 编译失败（变量也不行）
int x4{3.0};     // 编译失败！浮点→整型一律算窄化
```

本机实测 clang 23.1.0 报错原文：

```
error: type 'double' cannot be narrowed to 'int' in initializer list [-Wc++11-narrowing]
    int x2{3.14};
           ^~~~
note: insert an explicit cast to silence this issue
           static_cast<int>( )
```

注意 `int x4{3.0}` 也报错——标准里浮点→整型的转换**无条件**属于窄化，没有"常量可精确表示"的豁免（豁免只存在于浮点→浮点，如 `float{3.0}` 合法）。网上和旧笔记里"`int{3.0}` 合法"的说法是错的，以编译器为准。确实需要截断时，照编译器提示写 `static_cast<int>(d)`，把"我知道我在丢精度"写在脸上。

### 列表元素类型不一致也报错

`auto bad = {1, 2.0};` 实测报错 `error: deduced conflicting types ('int' vs 'double') for initializer list element type`——`initializer_list<T>` 要求单一元素类型，推不出唯一的 `T` 就失败（呼应 [Item 2](../ch01-deducing-types/item02-auto-type-deduction.md) 的 auto 大括号例外）。

---

## 最恼人解析（Most Vexing Parse）

```cpp
struct Widget { void hello() const {} };

Widget w1;      // OK：默认构造
Widget w2{};    // OK：{} 不会被解析成声明
Widget w3();    // 声明了一个"返回 Widget、无参"的函数！
// w3.hello(); // 解开注释会报错
```

本机实测 clang 23.1.0，编译期就有警告：

```
warning: empty parentheses interpreted as a function declaration [-Wvexing-parse]
    Widget w3();
             ^~
note: remove parentheses to declare a variable
```

如果解开 `w3.hello();` 的注释，报错更直白：

```
error: base of member reference is a function;
       perhaps you meant to call it with no arguments?
    w3.hello();
    ^~
       ()
```

"任何能被解析成声明的语法，就会被解析成声明"——`()` 空参数列表踩中这条规则，而 `{}` 在语法上不可能是函数声明，天然免疫。

---

## 模板代码里 () 与 {} 不可互换

写一个"完美创建对象"的工厂，把参数转发给 `T` 的构造函数：

```cpp
template<typename T, typename... Ts>
T makeParens(Ts&&... params) {
    return T(std::forward<Ts>(params)...);   // 内部用 ()
}

auto v1 = makeParens<std::vector<int>>(10, 20);   // size=10：10 个 20
std::vector<int> v2{10, 20};                      // size=2：{10, 20}
```

实测输出：`parens: size=10` / `braces: size=2`。模板作者**无法**替调用方决定语义：用 `()` 就没法表达"按列表初始化"，用 `{}` 就没法表达"按参数调构造"。这就是为什么 `std::make_shared`/`std::make_unique` 只能选 `()`，以及为什么标准库约定"花括号就是元素列表"。**转发参数的代码统一用 `()`，表示"列表"的代码显式用 `{}`，不要混。**

---

## 完整可运行例子

```cpp
#include <initializer_list>
#include <iostream>
#include <vector>

class Widget {
public:
    Widget() { std::cout << "default ctor\n"; }
    Widget(int i, bool b) { std::cout << "ctor(int,bool)\n"; }
    Widget(std::initializer_list<long double> il) {
        std::cout << "ctor(init_list), size=" << il.size() << "\n";
    }
};

int main() {
    Widget w0{};                       // 默认构造（Widget w0(); 会变成函数声明）
    Widget w1(1, true);                // ctor(int,bool)
    Widget w2{1, true};                // ctor(init_list)：劫持

    std::vector<int> v1(5, 42);        // 5 个 42
    std::vector<int> v2{5, 42};        // 两个元素
    std::vector<int> v3{};             // 空 {} = 默认构造
    std::cout << "v1: size=" << v1.size() << " front=" << v1.front() << "\n";
    std::cout << "v2: size=" << v2.size() << " front=" << v2.front() << "\n";
    std::cout << "v3.size()=" << v3.size() << "\n";
}
```

本机实测 clang 23.1.0（`-std=c++17 -Wall`）输出：

```
default ctor
ctor(int,bool)
ctor(init_list), size=2
v1: size=5 front=42
v2: size=2 front=5
v3.size()=0
```

---

## 常见错误（新手踩坑）

1. **`vector<int> v{10, 20}` 以为是"10 个 20"**。实测是 2 个元素 `{10, 20}`。规则：要 N 个相同值用 `(n, val)`，要逐个列元素用 `{}`。
2. **`Widget w();` 以为创建了对象**。它是函数声明，编译时 `-Wvexing-parse` 警告，调用成员时报 `base of member reference is a function`。修正：`Widget w;` 或 `Widget w{};`。
3. **以为 `int{3.0}` 能过**。实测 clang 23.1.0 报 `cannot be narrowed`——浮点→整型一律算窄化，常量也不例外。修正：`static_cast<int>(3.0)`。
4. **给类新增 `initializer_list` 构造函数后，老代码行为静默改变**。所有 `{}` 调用点会被劫持到新构造函数。修正：加 init_list 构造函数前 grep 一遍 `{}` 调用点，或改用 `static` 工厂函数（如 `Widget::fromList(...)`）避免重载集污染。

---

## HFT 关联

- **预分配 vs 元素列表**：`ring_buffer<Order> buf{capacity}` 是"一个元素，值是 capacity"（若 capacity 转得动 Order 就劫持，转不动才报错），`buf(capacity)` 才是预分配。低延迟代码里容器预分配写错，热身阶段就会踩到 reallocation。
- **防窄化拦真实 bug**：`int64_t qty{md.price}` 在 `price` 是 `double` 时编译失败，逼着写下 `static_cast`——下单数量被静默截断成整数的 bug 类被直接消灭。
- **聚合体 + `{}` 零成本初始化**：协议报文结构体 `struct alignas(64) Tick { int64_t ts; double bid, ask; };` 用 `Tick t{ts, bid, ask};` 聚合初始化，无构造函数调用开销，布局就是 ABI 布局。
- **工厂模板统一 `()`**：策略对象工厂 `template<class S, class... A> S makeStrategy(A&&... a)` 内部用 `S(std::forward<A>(a)...)`，保证语义只依赖构造函数签名，不被 init_list 劫持干扰。

---

## 自测题

**1. `vector<int> v(10, 20)` 和 `vector<int> v{10, 20}` 结果有何不同？根因是什么？**
<details><summary>答案</summary>
`(10, 20)` 得到 10 个值为 20 的元素；`{10, 20}` 得到 2 个元素 10 和 20。根因：`{}` 初始化时编译器**优先**尝试 `std::initializer_list` 构造函数，`{10, 20}` 整体匹配 `initializer_list<int>`；`()` 不参与这条优先规则，走普通重载决议匹配 `vector(size_type count, const T& value)`。
</details>

**2. `Widget w();` 声明的是什么？为什么 `Widget w{};` 就没这个问题？**
<details><summary>答案</summary>
声明了一个名为 `w`、无参数、返回 `Widget` 的函数（most vexing parse："能解析成声明就解析成声明"）。clang 实测给 `-Wvexing-parse` 警告。`{}` 在语法上不可能是函数声明，所以 `Widget w{};` 一定是默认构造一个对象。
</details>

**3. `int x{3.14};` 和 `int x{3.0};` 能编译通过吗？`int x(3.14);` 呢？**
<details><summary>答案</summary>
都不能。实测 clang 23.1.0 两行都报 `error: type 'double' cannot be narrowed to 'int' in initializer list`——浮点→整型无条件算窄化，`3.0` 常量也不例外。而 `int x(3.14)` 编译通过，静默截断为 3。这正是 `{}` 的价值：把静默截断变成编译期诊断。
</details>

**4. 类有 `Widget(int, bool)` 和 `Widget(std::initializer_list<long double>)` 两个构造函数，`Widget w{1, true};` 调哪个？**
<details><summary>答案</summary>
调 init_list 版本。实测输出 `ctor(init_list), size=2`：尽管 `(int, bool)` 是精确匹配，只要 `{}` 元素能转换成 init_list 的元素类型（`int`/`bool` → `long double` 非窄化），init_list 构造函数就劫持调用。
</details>

**5. 什么情况下 `{}` 不会走 `initializer_list` 构造函数？**
<details><summary>答案</summary>
两种：① 空 `{}`——走默认构造；② 元素类型无法转换成 init_list 元素类型时（如 `vector<string> vs{10, "hi"}`，`int` 转不了 `string`），退回普通重载决议，实测该例匹配 `(count, value)` 构造得到 10 个 `"hi"`。
</details>

**6. 为什么模板工厂函数里不能随意把 `T(args...)` 改成 `T{args...}`？**
<details><summary>答案</summary>
因为两者语义对同一组参数可能完全不同：`makeParens<vector<int>>(10, 20)` 用 `()` 得到 size=10，用 `{}` 得到 size=2（实测）。模板作者无法知道调用方想要哪种语义，所以标准工厂（`make_unique` 等）统一用 `()`；要表达列表初始化必须在调用点显式写 `{}`。
</details>

**7. `auto bad = {1, 2.0};` 会发生什么？**
<details><summary>答案</summary>
编译失败：`error: deduced conflicting types ('int' vs 'double') for initializer list element type`。`initializer_list<T>` 要求唯一元素类型，混合 `int`/`double` 推不出 `T`。
</details>

---

## 参考与延伸

- 上一节：[Item 6 auto 看不出来的显式类型初始化器](../ch02-auto/item06-explicitly-typed-initializer.md)
- 下一节：[Item 8 nullptr](item08-nullptr.md)
- 相关：[Item 2 auto 类型推导](../ch01-deducing-types/item02-auto-type-deduction.md)（auto + `{}` 的 initializer_list 规则）
- 回到：[第 3 章 移步现代 C++](README.md)
