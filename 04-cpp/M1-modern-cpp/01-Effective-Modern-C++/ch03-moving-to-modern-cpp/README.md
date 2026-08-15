# 第 3 章 移步现代 C++

**Moving to Modern C++** — Items 7–17

## 本章讲什么

C++11/14 在 C++ 基础语法层做了一大批改进：初始化、空指针、类型别名、枚举、特殊成员函数、`noexcept`、`constexpr`……这些不是孤立的语法点，而是相互咬合的"现代基线"。本章逐 item 给出"为什么要换 + 怎么换 + 坑在哪"。

---

## 各 Item 要点

### Item 7：区别 `()` 和 `{}` 创建对象

C++11 有了大括号初始化 `{}`，它几乎能初始化一切，但有别于小括号 `()`：

| 特性 | `()` | `{}` |
|------|------|------|
| 窄化转换 | 允许（`int(3.14)`） | **禁止**（`int{3.14}` 编译失败） |
| 最烦人解析 | 会中招（`Widget w();` 被解析成函数声明） | 免疫 |
| `initializer_list` 构造函数 | 不优先 | **优先匹配** |

**`initializer_list` 优先陷阱**：
```cpp
std::vector<int> v1(10, 20);   // 10 个 20 → {20,20,...,20}
std::vector<int> v2{10, 20};   // 2 个元素 → {10, 20}  匹配 initializer_list 构造
```
`{}` 会**优先**匹配 `initializer_list` 构造函数，哪怕有更优匹配。空 `{}` 则调用默认构造（不会造出空的 `initializer_list`）。

**建议**：对内置类型用 `{}` 防窄化；对容器/自定义类，先查它有没有 `initializer_list` 构造，再决定用 `{}` 还是 `()`。

### Item 8：优先 `nullptr` 而非 `0` 和 `NULL`

`0` 和 `NULL` 都是整型字面量，在指针与整型重载时会**误选整型重载**：
```cpp
void f(int);   void f(Widget*);
f(0);          // 调 f(int)！不是 f(Widget*)
f(NULL);       // 仍可能调 f(int)（NULL 的类型依赖实现）
f(nullptr);    // 调 f(Widget*)，正确
```
`nullptr` 的类型是 `std::nullptr_t`，能隐式转任意指针，**不能转整型**——彻底消除指针/整型重载歧义。模板推导里也能正确推导出指针类型。

### Item 9：优先 `using` 别名而非 `typedef`

`using` 支持模板化（alias template），`typedef` 不行：
```cpp
template<class T> using Vec = std::vector<T, MyAlloc<T>>;  // OK
template<class T> typedef std::vector<T, MyAlloc<T>> Vec;  // 编译失败
```
`using` 还能避免读复杂的函数指针类型，且与 C++14 `auto` 返回类型推导配合更自然。

### Item 10：优先限定作用域枚举（scoped enum）

```cpp
enum Color { Red, Green };        // unscoped：Red 泄漏到外层作用域
enum class Color2 { Red, Green }; // scoped：必须 Color2::Red
```
限定作用域枚举（`enum class`）不隐式转整型、不污染外层命名空间、可前向声明（可指定底层类型）。HFT 协议字段用 `enum class` 防止误比较。

### Item 11：优先 `= default` 声明默认构造

`= default` 让编译器生成默认实现，比手写空函数体更可靠（手写 `C() {}` 会变成"用户定义"，影响是否为 trivial 类型——影响 `memcpy` 合法性与 ABI）。

### Item 12：把重写函数声明为 `override`

`override` 关键字让编译器**检查**是否真的重写了基类虚函数——签名不匹配（const 差异、参数类型差异、引用限定符差异）会编译报错，而非静默创建一个新虚函数。这是 C++11 最具性价比的防 bug 特性之一。

### Item 13：优先 `const_iterator` 而非 `iterator`

C++11 引入 `cbegin()`/`cend()`，配合 `auto`：`auto it = v.cbegin();` 拿到 `const_iterator`，防止意外修改。STL 算法（`find`/`insert`）在 C++14 起支持 `const_iterator`。

### Item 14：声明 `noexcept` 如果函数保证不抛

`noexcept` 是函数接口契约的一部分。对**移动构造**、**swap**、**析构**标 `noexcept` 尤其关键——STL 容器在 `push_back` 扩容时会检查元素移动构造是否 `noexcept`：是则用移动（快），否则退回拷贝（安全）。标错 `noexcept` 但抛异常会 `std::terminate`。

### Item 15：尽可能用 `constexpr`

`constexpr` 表示"编译期可求值"。`constexpr` 对象是编译期常量；`constexpr` 函数在编译期能求值时就编译期求值，否则退化为运行时。C++14 起 `constexpr` 函数可用 `if`/局部变量/循环，能力大增。HFT 用 `constexpr` 做编译期查表、协议字段偏移计算，零运行开销。

### Item 16：让 `const` 成员函数线程安全

`const` 成员函数仍可修改 `mutable` 成员（如缓存、互斥锁）。如果 `const` 函数会读写 `mutable` 成员，它就不是天然线程安全的——必须加锁或用 `std::atomic`。否则两个线程同时调用同一个 `const` 对象的该方法会数据竞争。

### Item 17：理解特殊成员函数的生成规则

C++11 的"大三律"扩展为"大五律"（拷贝构造、拷贝赋值、移动构造、移动赋值、析构）。生成规则：**声明了任何一个，其余的生成受抑制**。具体而言，声明了拷贝操作会抑制移动操作的自动生成；声明了析构会抑制移动生成（C++11）但保留拷贝生成（已废弃，C++11 起警告）。手写任何一个就应该把五个都写（或 `=default`/`=delete`）。

---

## HFT 关联

- **`noexcept` 与 STL 扩容**：热路径里 `vector<Order> push_back` 扩容时，`Order` 的移动构造必须 `noexcept` 才走移动语义——否则扩容退回拷贝，订单簿重建延迟尖峰。这是 HFT C++ 性能的隐形开关。
- **`constexpr` 编译期求值**：协议字段偏移、校验和表、费率表用 `constexpr` 编译期算好，运行时零开销。比 C 的 `#define` / `static const` 更强（可用于模板参数、`static_assert`）。
- **`enum class` 防误比较**：订单状态、Side(Buy/Sell) 用 `enum class`，编译器拒绝 `Side::Buy == 1` 的隐式比较，消除一类下单逻辑 bug。
- **`override` 检查**：策略基类的虚函数（`on_tick`/`on_fill`）加 `override`，子类签名写错编译期暴露，避免"虚函数没被重写、策略不生效"的隐蔽 bug。

---

## 自测题

1. `vector<int> v(10, 20)` 和 `vector<int> v{10, 20}` 结果有何不同？根因是什么？
2. `f(0)` 在 `void f(int); void f(Widget*);` 重载集里调用哪个？`f(nullptr)` 呢？
3. 声明了移动构造后，拷贝构造还会自动生成吗？声明了析构呢（C++11）？
4. STL 容器 `push_back` 扩容时如何决定用移动还是拷贝？`noexcept` 在其中起什么作用？
5. `const` 成员函数为什么可能不是线程安全的？`mutable` 在其中扮演什么角色？



## 代码自测

### Q1: 花括号 vs 圆括号

```cpp
std::vector<int> v1(10, 20);   // A
std::vector<int> v2{10, 20};   // B
```

> v1 和 v2 分别是什么？为什么不同？

<details>
<summary>答案与复习指引</summary>

- v1 = 10 个元素，每个值 20（`(count, value)` 构造）
- v2 = 2 个元素 {10, 20}（花括号优先匹配 `initializer_list` 构造）

**根因：** `{}` 会**优先**匹配 `initializer_list` 构造函数，即使有更精确匹配的 `(int, int)` 构造函数。

**花括号还禁止窄化：** `int x{3.14};` 编译失败（double→int 窄化），`int x(3.14);` 合法（静默截断）。

**复习：** → [Item 7：区别 () 和 {} 创建对象](item07-parens-vs-braces.md)
</details>

### Q2: noexcept 与 vector 扩容

```cpp
class Widget {
public:
    // 版本 A: 没标 noexcept
    Widget(Widget&& o) { /* 移动 */ }
    // 版本 B: 标了 noexcept
    // Widget(Widget&& o) noexcept { /* 移动 */ }
};
std::vector<Widget> v;
for (int i = 0; i < 100; ++i)
    v.push_back(Widget(i));
```

> 版本 A 和 B 在扩容时分别用移动还是拷贝？

<details>
<summary>答案与复习指引</summary>

- **版本 A（无 noexcept）：** STL 退回**拷贝**——因为移动构造可能抛异常，扩容中途异常会导致数据丢失。拷贝保证强异常安全。
- **版本 B（有 noexcept）：** STL 用**移动**——O(1) 指针交接，扩容从 O(n) 降到 O(n) 但每个元素是 O(1) 移动而非 O(n) 拷贝。

**这是 HFT 性能的隐形开关：** 移动构造必须标 `noexcept`，否则 `vector` 扩容退回拷贝。

**复习：** → [Item 14：声明 noexcept 如果函数保证不抛](item14-noexcept.md)
</details>

### Q3: override 检查

```cpp
class Base {
public:
    virtual void f(int x) const {}
};
class Derived : public Base {
public:
    void f(int x) override {}  // 编译成功吗？
};
```

> 这段代码编译成功吗？为什么？

<details>
<summary>答案与复习指引</summary>

**编译失败。** `Derived::f` 缺少 `const` 限定符——签名与 `Base::f` 不匹配。`override` 让编译器检查：签名必须完全匹配（const、参数类型、引用限定符）。没有 `override`，这会静默创建一个新虚函数而非重写。

**教训：** 重写虚函数时始终加 `override`，让编译器帮你检查签名。

**复习：** → [Item 12：把重写函数声明为 override](item12-override.md)
</details>
