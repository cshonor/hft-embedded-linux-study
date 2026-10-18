# Item 24：区分万能引用和右值引用

> 第 5 章 右值引用、移动语义与完美转发 · Item 24 · 上一节：[Item 23 move/forward](item23-move-and-forward.md) · 下一节：[Item 25 move vs forward 使用](item25-move-vs-forward-usage.md)

## 为什么要学这个（先建立直觉）

`T&&` 这个语法在 C++11 之后有两个完全不同的身份，长得一模一样：

- 有时候它是**右值引用**——"只绑右值，我要接管一个将死的对象"；
- 有时候它是**万能引用**——"左右值通吃，我先把实参的类型和值类别记下来待会原样转发"。

分不清这两个身份，你就读不懂任何一段泛型代码的接口语义：`template<class T> void f(T&&)` 和 `void f(Widget&&)` 的形参看着只差一个字母，行为却是"来者不拒"和"只收右值"的天壤之别。更糟的是判断规则不在语法长相上，而在**语境**里——这一节就是给你那套判断规则。

---

## 这节讲什么

1. 两问判断法（缺一不可），配反例与实测报错原文；
2. `__PRETTY_FUNCTION__` 推导实测：数组、函数、字符串字面量、const 左值、容器等更多形态；
3. 常见"假万能引用"：`const T&&`、`std::vector<T>&&`、类模板成员里的 `T&&`；
4. `auto&&` 与模板 `T&&` 的等价性（含 C++20 缩略函数模板）；
5. 与 `std::move` / `std::forward` 的衔接——万能引用不 `forward` 会怎样；
6. 全部论断附本机实测（clang 23.1.0，arm64-apple-darwin，libc++，`-std=c++17 -Wall`）。

---

## 两问判断法

看到 `&&` 先问两个问题，**两个都答"是"才是万能引用**：

| # | 问题 | 答"否"的例子 |
|---|---|---|
| 1 | **有类型推导吗？**（`T` 是本函数模板待推导的参数，或 `auto`） | `void f(Widget&&)`、`template<class T> class C { void f(T&&); }` |
| 2 | **形式恰好是裸 `T&&` 吗？**（不能有任何修饰或包裹） | `const T&&`、`std::vector<T>&&` |

```cpp
void f(Widget&& w);               // 右值引用：无推导（问题 1 答否）
template<class T> void g(T&& x);  // 万能引用：有推导 + 裸 T&&
auto&& x = expr;                  // 万能引用：auto 也是推导
template<class T> void h(const T&& x);        // 右值引用：有 const（问题 2 答否）
template<class T> void k(std::vector<T>&& v); // 右值引用：&& 不在裸 T 上（问题 2 答否）
template<class T> class C { void m(T&& x); }; // 右值引用：T 在类实例化时已定（问题 1 答否）
```

### 反例实测：全都接不了左值

逐个用**左值实参**调用，clang 23.1.0 报错原文：

```
// void f(Widget&& w);  f(w);
error: no matching function for call to 'f'
note: candidate function not viable: expects an rvalue for 1st argument

// template<class T> void f(const T&& x);  f(x);   (x 是 int)
error: no matching function for call to 'f'
note: candidate function [with T = int] not viable: expects an rvalue for 1st argument

// template<class T> void f(std::vector<T>&& v);  f(v);
error: no matching function for call to 'f'
note: candidate function [with T = int] not viable: expects an rvalue for 1st argument

// template<class T> class C { void f(T&& x); };  C<int> c; int x; c.f(x);
error: rvalue reference to type 'int' cannot bind to lvalue of type 'int'
```

注意最后一条的报错措辞不同——因为类模板成员函数里 `T` 早已确定，`T&&` 就是纯粹的 `int&&`，clang 直接给"右值引用绑不上左值"的诊断，连重载决议都不用走。**"模板 + `&&`"从来不是万能引用的充分条件。**

对照组：`template<class T> void f(T&& x)` 传左值和右值都编译通过。

---

## 万能引用的推导实测

用 `__PRETTY_FUNCTION__` 观察 `T` 的真实推导结果（clang 23.1.0）：

```cpp
template <typename T>
void g(T&& x) {
    std::cout << "  " << __PRETTY_FUNCTION__ << "\n";
    std::cout << "     形参是左值引用? " << std::is_lvalue_reference<decltype(x)>::value
              << "   形参是 const? "
              << std::is_const<typename std::remove_reference<decltype(x)>::type>::value << "\n";
}
```

**内置类型：**

| 实参 | `T` 的推导 | 形参类型 |
|---|---|---|
| `g(x)`（`int` 左值） | `T = int &` | `int&`（左值引用） |
| `g(cx)`（`const int` 左值） | `T = const int &` | `const int&` |
| `g(lref)`（`int&` 变量） | `T = int &` | `int&` |
| `g(42)`（纯右值） | `T = int` | `int&&` |
| `g(std::move(x))`（将亡值） | `T = int` | `int&&` |
| `g(3.14)` | `T = double` | `double&&` |

**数组与函数**（这是最能说明"左值也带类型信息"的一组）：

| 实参 | `T` 的推导 |
|---|---|
| `g(arr)`（`int[5]`） | `T = int (&)[5]` ← 数组类型+长度完整保留，不退化成指针 |
| `g(carr)`（`const int[3]`） | `T = const int (&)[3]` |
| `g(freeFunc)`（`void(int)`） | `T = void (&)(int)` ← 函数左值，推成函数引用 |
| `g("literal")` | `T = const char (&)[8]` ← 含结尾 `\0` 的长度 8 |

**类类型与容器**：`std::string` 左值 → `T = std::string &`；`const std::string` 左值 → `T = const std::string &`；`std::string("tmp")` 与 `std::move(s)` → `T = std::string`；`std::vector<int>` 左值 → `T = std::vector<int> &`，`std::vector<int>{4,5}` → `T = std::vector<int>`。

规则就两行：

- **左值实参 → `T` 推为 `T&`**，形参类型 `T&&` = `T& &&`，引用折叠成 `T&`（折叠规则见 [Item 28](item28-reference-collapsing.md)）；
- **右值实参 → `T` 推为 `T`**，形参类型 `T&&`。

"T 自己变成引用类型"这条特殊规则只在万能引用推导里存在——这是它和普通模板推导的分叉点，也是"万能"的全部秘密。数组/函数那几行额外说明一件事：**万能引用不触发数组到指针、函数到指针的退化**，因为实参是左值，`T` 直接带上引用。

---

## auto&& 与 T&& 等价

`auto&&` 走的是和模板 `T&&` **完全相同**的推导（[Item 2](../ch01-deducing-types/item02-auto-type-deduction.md) 已实测）：

```cpp
int i = 0;
auto&& r1 = i;              // int&
auto&& r2 = 42;             // int&&
auto&& r3 = std::move(i);   // int&&
const auto&& r4 = 42;       // const int&& —— 加了 const 就不是万能引用了
```

实测 `decltype(r1)==int&` → 1、`decltype(r2)==int&&` → 1、`decltype(r3)==int&&` → 1、`decltype(r4)==const int&&` → 1；另外 `decltype((r1))` 是 `int&`——**具名变量永远是左值表达式**，哪怕它的类型是 `int&&`。这条是理解"为什么万能引用形参必须 `forward`"的关键。

所以读代码时 `auto&&` 一律按万能引用处理。常见用途：范围 for 里 `for (auto&& elem : range)`——容器元素是代理（`vector<bool>`）或引用时也能正确绑定。C++20 的缩略函数模板 `void h(auto&& x)` 同理：实测推导 `[x:auto = int &]` / `[x:auto = const int &]` / `[x:auto = int]`，与模板版逐字对应；clang 在 `-std=c++11/14/17` 下给 `warning: 'auto' parameters are a C++20 extension [-Wc++20-extensions]`。

---

## 与 move / forward 的衔接

万能引用是"入口"，正确处理一定配 `std::forward`。三种写法实测（构造/拷贝/移动带打印）：

```cpp
template <class T> void good(T&& x)   { Traced t(std::forward<T>(x)); }  // 值类别原样传
template <class T> void bad(T&& x)    { Traced t(x); }                   // 不用 forward
template <class T> void greedy(T&& x) { Traced t(std::move(x)); }        // 无条件 move
```

```
good(lval)  ← 左值实参
    COPY ctor
good(Traced{}) ← 右值实参
    MOVE ctor
bad(lval)
    COPY ctor
bad(Traced{})       ← 右值也退化成拷贝！
    COPY ctor
greedy(lval) ← 左值也被 move，lval.s 被掏空
    MOVE ctor
  lval.s 现在为空? yes
```

- `good`：左值走拷贝、右值走移动，**这才是完美转发**（详见 [Item 25](item25-move-vs-forward-usage.md)）；
- `bad`：形参 `x` 是具名变量、永远是左值表达式，右值的移动性在函数体内**丢失**——写了万能引用却不用 `forward`，等于把接口写成"能接一切但一律拷贝"；
- `greedy`：对左值也无条件 `std::move`，实测把调用方的 `lval.s` 掏空了——这是最危险的写法，调用方完全没料到自己的左值被搬走。

记忆：**万能引用形参在函数体内的每一次"往下传"，都该是 `std::forward<T>(x)`；只有"这是我最后一次用它"才用 `std::move(x)`，且只在你确定可以搬走时。**

### 重载决议：非模板右值引用优先

实测（`void f(Widget&&)` 与 `template<class T> void f(T&&)` 并存）：`f(w)`（左值）→ 万能引用版；`f(Widget{})` 和 `f(std::move(w))`（右值）→ **非模板右值引用版**；`f(s)`（`std::string`）→ 万能引用版。

规则和其他重载一样：**两者都能精确匹配时，非模板优先**。这也意味着给万能引用再加一个 `Widget&&` 重载，会把右值流量全部截走——Meyers 在 Item 26 专门讲为什么**不要**这么重载万能引用（见 [Item 26](item26-avoid-overloading-universal.md)）。

---

## 完整可运行例子

```cpp
#include <iostream>
#include <string>
#include <utility>
#include <vector>

struct Widget {};
void rvRef(Widget&&)               { std::cout << "  右值引用版：只收 Widget 右值\n"; }
template<class T> void uniRef(T&&) { std::cout << "  万能引用版：来者不拒\n"; }
template<class T> void vecRef(std::vector<T>&&) { std::cout << "  vector<T>&&：只收 vector 右值\n"; }

struct Payload {
    Payload() = default;
    Payload(const Payload&) { std::cout << "    COPY\n"; }
    Payload(Payload&&) noexcept { std::cout << "    MOVE\n"; }
};
template <class T> void wrap(T&& x) { Payload p(std::forward<T>(x)); (void)p; }

int main() {
    std::cout << std::unitbuf;
    Widget w;
    std::cout << "== 接口接受面 ==\n";
    uniRef(w);            uniRef(Widget{});
    rvRef(Widget{});      // rvRef(w);  ← 解开注释会编译失败
    std::vector<int> v;
    vecRef(std::vector<int>{1, 2});   // vecRef(v);  ← 解开注释会编译失败

    std::cout << "== forward 的价值 ==\n";
    Payload p;
    std::cout << "  wrap(左值) →"; wrap(p);
    std::cout << "  wrap(右值) →"; wrap(Payload{});

    std::cout << "== auto&& 也是万能引用 ==\n";
    int i = 0;
    auto&& r1 = i;      // int&
    auto&& r2 = 42;     // int&&
    std::cout << "  decltype(r1)==int&? " << std::is_same<decltype(r1), int&>::value
              << "   decltype(r2)==int&&? " << std::is_same<decltype(r2), int&&>::value << "\n";
}
```

本机实测 clang 23.1.0（`-std=c++17 -Wall`）输出：

```
== 接口接受面 ==
  万能引用版：来者不拒
  万能引用版：来者不拒
  右值引用版：只收 Widget 右值
  vector<T>&&：只收 vector 右值
== forward 的价值 ==
  wrap(左值) →    COPY
  wrap(右值) →    MOVE
== auto&& 也是万能引用 ==
  decltype(r1)==int&? 1   decltype(r2)==int&&? 1
```

---

## 常见错误（新手踩坑）

1. **看 `&&` 就当移动语义**。`template<class T> void f(T&&)` 不表达"我要移动你"，它表达"我什么都要"——对左值实参它绑左值引用，函数里直接改形参会改到调用方的对象。移动意图只在**右值引用**（`Widget&&`）里存在。
2. **万能引用形参不 `forward` 就用**。形参 `x` 是变量名，变量名永远是左值表达式。实测 `bad(Traced{})` 打出的是 `COPY ctor`——右值的移动性丢了。
3. **给万能引用加 const 优化**。`const T&&` 不是"更安全的万能引用"，是"只收 const 右值的窄接口"，实测接非 const 左值报 `no matching function for call`。
4. **类模板的成员函数上误认万能引用**。`template<class T> class C { void f(T&& x); };` ——`T` 在**类实例化时**已确定，`f` 处无推导，实测报 `rvalue reference to type 'int' cannot bind to lvalue of type 'int'`。要万能引用得自己再开模板：`template<class U> void f(U&&)`。
5. **对万能引用形参无脑 `std::move`**。实测把调用方的左值掏空（`lval.s` 变空），是静默的数据破坏。

---

## HFT 关联

- **泛型回调注册**：`template<class F> void setCallback(F&& f)` 的 `F&&` 是万能引用——lambda、函数指针、仿函数来者不拒，配合 `std::forward<F>` 存进类型擦除容器。muduo 的 `setReadCallback` 就是这个形态。
- **接口语义审查**：评审泛型接口时，看到 `T&&` 先按两问检查定性——把右值引用当万能引用理解会误判接口的接受面，把万能引用当右值引用理解会漏掉"左值也能进"的副作用面。
- **`for (auto&& e : book)`**：遍历订单簿时用 `auto&&` 而不是 `auto`——前者绑引用零拷贝，后者拷贝每个订单对象。
- **行情字段转发**：`template<class T> void push(T&& tick)` 必须内部 `std::forward<T>(tick)` 落地，否则右值 tick 会在入队时白白多一次深拷贝——在百万 tick/秒的路径上这笔拷贝就是延迟。

---

## 自测题

**1. `void f(Widget&&)` 和 `template<class T> void g(T&&)` 的 `&&` 有何本质不同？**
<details><summary>答案</summary>
`void f(Widget&&)` 是普通右值引用：`Widget` 类型确定、无推导，只绑右值（实测接左值报 `no matching function for call to 'f'`），表达"接管将死对象"。`g(T&&)` 是万能引用：`T` 待推导，接左值时 `T` 推为 `T&` 经引用折叠形参变 `T&`，接右值时 `T` 推为 `T`、形参为 `T&&`——同一个函数左右值通吃，配合 `std::forward<T>` 还原值类别。
</details>

**2. 万能引用的两条判断标准是什么？各举一个"差一点但不是"的反例。**
<details><summary>答案</summary>
①发生类型推导（`T` 是函数模板自己推导的参数，或 `auto`）；②形式恰好是裸 `T&&`。反例：满足①不满足②——`std::vector<T>&&`（`&&` 不在裸 T 上，实测接左值报 `expects an rvalue`）；满足②不满足①——`template<class T> class C { void f(T&&); }`（`T` 在类实例化时已定，实测报 `rvalue reference to type 'int' cannot bind to lvalue of type 'int'`）。`const T&&` 则连形式都不满足。
</details>

**3. 万能引用接数组、函数、字符串字面量时 `T` 推成什么？**
<details><summary>答案</summary>
实测 `g(arr)` → `T = int (&)[5]`（数组类型和长度完整保留，**不退化成指针**）；`g(carr)` → `T = const int (&)[3]`；`g(freeFunc)` → `T = void (&)(int)`（函数左值推成函数引用）；`g("literal")` → `T = const char (&)[8]`（长度含结尾 `\0`）。因为这些都是左值，触发"左值 → `T` 带引用"的规则。
</details>

**4. `auto&&` 是什么引用？`const auto&&` 呢？`for (auto&& e : v)` 相比 `for (auto e : v)` 的意义？**
<details><summary>答案</summary>
`auto&&` 是万能引用（`auto` 推导等价于模板推导）。实测 `decltype(r1)==int&`、`decltype(r2)==int&&`、`decltype(r3)==int&&`（绑 `std::move(i)`）。`const auto&&` 加了 const，不满足"恰好 `T&&`"，实测 `decltype(r4)==const int&&`，是纯右值引用。`for (auto&& e : v)` 绑元素引用，零拷贝且能正确处理代理元素（`vector<bool>::reference`）；`for (auto e : v)` 逐元素拷贝——遍历订单簿时后者是每 tick 全簿深拷贝。
</details>

**5. 类模板的成员函数 `template<class T> class C { void f(T&&); };` 里 `f` 的形参是万能引用吗？**
<details><summary>答案</summary>
不是。`T` 是**类**模板参数，在 `C<Widget>` 实例化时已确定；成员函数 `f` 用到 `T` 时没有推导发生，`T&&` 是纯右值引用。实测 `C<int> c; int x; c.f(x);` 报 `error: rvalue reference to type 'int' cannot bind to lvalue of type 'int'`。要万能引用，`f` 必须自己开模板：`template<class U> void f(U&&)`，`U` 在调用处推导。
</details>

**6. 万能引用形参不 `std::forward` 直接用会怎样？无脑 `std::move` 又会怎样？**
<details><summary>答案</summary>
不 `forward`：形参 `x` 是具名变量，永远是左值表达式，右值实参的移动性丢失——实测 `bad(Traced{})` 打出 `COPY ctor` 而不是 `MOVE ctor`。无脑 `std::move`：左值也被搬走，实测 `greedy(lval)` 之后调用方的 `lval.s` 变成空字符串，是静默的数据破坏。正解是 `std::forward<T>(x)`，实测左值走 COPY、右值走 MOVE。
</details>

**7. `void f(Widget&&)` 和 `template<class T> void f(T&&)` 同时存在，传右值调哪个？为什么？**
<details><summary>答案</summary>
调非模板的 `f(Widget&&)`。实测 `f(Widget{})` 和 `f(std::move(w))` 都走"右值引用版"，只有左值 `f(w)` 和完全无关的类型 `f(s)`（`std::string`）才走万能引用版。规则：两者都能精确匹配时**非模板优先**。这也意味着给万能引用额外加一个 `Widget&&` 重载会截走全部右值流量——所以 Meyers 建议不要重载万能引用（见 [Item 26](item26-avoid-overloading-universal.md)）。
</details>

---

## 参考与延伸

- 上一节：[Item 23 move/forward](item23-move-and-forward.md)
- 下一节：[Item 25 move vs forward 使用](item25-move-vs-forward-usage.md)
- 相关：[Item 28 引用折叠](item28-reference-collapsing.md)（`T& &&` 为什么折叠成 `T&`）、[Item 2 auto 类型推导](../ch01-deducing-types/item02-auto-type-deduction.md)
- 回到：[第 5 章 右值引用、移动语义与完美转发](README.md)
