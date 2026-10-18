# Item 24：区分万能引用和右值引用

> 第 5 章 · Item 24 · 上一节：[Item 23 move/forward](item23-move-and-forward.md)

## 为什么要学这个（先建立直觉）

`T&&` 这个语法在 C++11 之后有两个完全不同的身份，长得一模一样：

- 有时候它是**右值引用**——"只绑右值，我要接管一个将死的对象"；
- 有时候它是**万能引用**——"左右值通吃，我先把实参的类型和值类别记下来待会原样转发"。

分不清这两个身份，你就读不懂任何一段泛型代码的接口语义：`template<class T> void f(T&&)` 和 `void f(Widget&&)` 的形参看着只差一个字母，行为却是"来者不拒"和"只收右值"的天壤之别。更糟的是判断规则不在语法长相上，而在**语境**里——这一节就是给你那套判断规则。

---

## 这节讲什么

1. 万能引用的两条判断标准（缺一不可），附反例；
2. 左值/右值实参下 `T` 的推导结果实测；
3. `const T&&`、`std::vector<T>&&` 为什么不是万能引用；
4. `auto&&` 与模板 `T&&` 的等价性。

---

## 核心区别：同一张脸，两个身份

```cpp
void f(Widget&& w);               // 右值引用：无推导，只绑右值
template<class T> void g(T&& x);  // 万能引用：有推导，左右值都绑
auto&& x = expr;                  // 万能引用：auto 也是推导
```

判断标准（**两条同时满足**才是万能引用）：

1. **发生类型推导**：`T` 是待推导的模板参数，或者是 `auto`。`Widget&&` 类型已定，没有推导，直接出局。
2. **形式恰好是 `T&&`**：不能有任何修饰或包裹。`const T&&` 出局，`std::vector<T>&&` 出局（`&&` 不在裸 `T` 上）。

反例验证（clang 23.1.0）：

```cpp
struct Widget {};
void f(Widget&& w);
Widget w;
f(w);
// error: no matching function for call to 'f'
// 右值引用不绑左值——毫无商量

template<typename T> void h(const T&& x);
int x = 0;
h(x);
// error: no matching function for call to 'h'
// const T&& 是右值引用：只绑 const 右值，非 const 左值不要
```

## 万能引用的推导实测

`__PRETTY_FUNCTION__` 观察 `T` 的真实推导结果（clang 23.1.0）：

```cpp
template<typename T>
void g(T&& x) { std::cout << __PRETTY_FUNCTION__ << "\n"; }

int x = 0;
const int cx = 0;
g(x);    // void g(T &&) [T = int &]        ← 左值：T 推成引用
g(cx);   // void g(T &&) [T = const int &]  ← const 左值：const 也进 T
g(42);   // void g(T &&) [T = int]          ← 右值：T 推成纯类型
```

规则就两行：

- **左值实参 → `T` 推为 `T&`**，形参类型 `T&&` = `T& &&`，引用折叠成 `T&`（折叠规则见 [Item 28](item28-reference-collapsing.md)）；
- **右值实参 → `T` 推为 `T`**，形参类型 `T&&`。

"T 自己变成引用类型"这条特殊规则只在万能引用推导里存在——这是它和普通模板推导的分叉点，也是"万能"的全部秘密。

## auto&& 与 T&& 等价

`auto&&` 走的是和模板 `T&&` 完全相同的推导（[Item 2](../ch01-deducing-types/item02-auto-type-deduction.md) 已实测）：

```cpp
auto&& uref1 = x;    // x 是 int 左值 → decltype(uref1) == int&
auto&& uref2 = 27;   // 右值        → decltype(uref2) == int&&
```

所以读代码时 `auto&&` 一律按万能引用处理。常见用途：范围 for 里 `for (auto&& elem : range)`——容器元素是代理（`vector<bool>`）或引用时也能正确绑定。

## `std::vector<T>&&` 为什么不是万能引用

```cpp
template<class T>
void bad(std::vector<T>&& v);   // 右值引用！只接 vector 右值
```

`T` 确实在推导，但 `&&` 挂在 `std::vector<T>` 上而不是裸 `T` 上——形式不是 `T&&`，第二条不满足。这个函数只接受 `vector` 的右值，传左值 `vector` 编译失败。这个坑在泛型容器接口里很常见：以为写了"模板 + &&"就是万能引用，其实收的是纯右值。

---

## 常见错误（新手踩坑）

1. **看 `&&` 就当移动语义**。`template<class T> void f(T&&)` 不表达"我要移动你"，它表达"我什么都要"——对左值实参它绑左值引用，函数里直接改形参会改到调用方的对象。移动意图只在**右值引用**（`Widget&&`）里存在。
2. **万能引用形参不 `forward` 就用**。形参 `x` 是变量名，变量名永远是左值表达式——哪怕它绑的是右值。不把 `std::forward<T>(x)` 写上，右值的移动性在函数体内就丢了（Item 25 详述）。
3. **给万能引用加 const 优化**。`const T&&` 不是"更安全的万能引用"，是"只收 const 右值的窄接口"——语义完全变了。
4. **类模板的成员函数上误认万能引用**。`template<class T> class C { void f(T&& x); };` —— `T` 在**类实例化时**已确定，`f` 的 `T&&` 无推导发生，是右值引用。成员函数要万能引用得自己再开模板：`template<class U> void f(U&&)`。

---

## 新手要点（和 C 的区别）

- **C 没有引用**，只有指针。C 程序员学引用先建立"引用 = 别名，不是对象"的直觉，再谈左右值。
- **看到 `T&&` 先问两个问题**：这里有推导吗？形式是裸 `T&&` 吗？两个都答"是"才是万能引用，否则是右值引用。这个两问检查应该变成肌肉记忆。
- **万能引用 ≠ 万能药**：它只是"能绑一切"的入口，正确用法永远配合 `std::forward`——否则你写的就是个"来者不拒但乱处理"的函数。

---

## HFT 关联

- **泛型回调注册**：`template<class F> void setCallback(F&& f)` 的 `F&&` 是万能引用——lambda、函数指针、仿函数来者不拒，配合 `std::forward<F>` 存进类型擦除容器。muduo 的 `setReadCallback` 就是这个形态。
- **接口语义审查**：评审泛型接口时，看到 `T&&` 先按两问检查定性——把右值引用当万能引用理解会误判接口的接受面，把万能引用当右值引用理解会漏掉"左值也能进"的副作用面。
- **`for (auto&& e : book)`**：遍历订单簿时用 `auto&&` 而不是 `auto`——前者绑引用零拷贝，后者拷贝每个订单对象。

---

## 自测题

1. `void f(Widget&&)` 和 `template<class T> void g(T&&)` 的 `&&` 有何本质不同？
2. 万能引用的两条判断标准是什么？各举一个"差一点但不是"的反例。
3. `const T&&` 是万能引用吗？实测它接非 const 左值会怎样？
4. 万能引用接左值时 `T` 推成什么？接右值时呢？（用 `__PRETTY_FUNCTION__` 实测结果说明）
5. `template<class T> class C { void f(T&&); };` 里 `f` 的形参是万能引用吗？为什么？
6. `auto&&` 是什么引用？`for (auto&& e : v)` 相比 `for (auto e : v)` 的意义？

<details>
<summary>参考答案</summary>

1. `void f(Widget&&)` 是普通右值引用：`Widget` 类型确定、无推导，只绑右值（实测接左值报 `no matching function for call to 'f'`），表达"接管将死对象"。`g(T&&)` 是万能引用：`T` 待推导，接左值时 `T` 推为 `T&` 经引用折叠形参变 `T&`，接右值时 `T` 推为 `T`、形参为 `T&&`——同一个函数左右值通吃，配合 `std::forward<T>` 还原值类别。

2. ①发生类型推导（`T` 是函数模板自己推导的参数，或 `auto`）；②形式恰好是裸 `T&&`。反例：满足①不满足②——`std::vector<T>&&`（`&&` 不在裸 T 上）；满足②不满足①——`template<class T> class C { void f(T&&); }`（`T` 在类实例化时已定，`f` 处无推导）。`const T&&` 则连形式都不满足。

3. 不是。多了 `const` 修饰就不是"恰好 `T&&`"，是右值引用，只能绑 const 右值。实测 `h(const T&&)` 接非 const 左值报 `no matching function for call to 'h'`。

4. 接左值 `T` 推为 `int&`（const 左值则 `const int&`），接右值 `T` 推为 `int`。实测输出：`g(x)` → `[T = int &]`，`g(cx)` → `[T = const int &]`，`g(42)` → `[T = int]`。左值让 `T` 带引用，形参 `T&&` 折叠成 `T&` 所以能绑左值——这是万能引用的全部机制。

5. 不是。`T` 是**类**模板参数，在 `C<Widget>` 实例化时已确定；成员函数 `f` 用到 `T` 时没有推导发生，`T&&` 是纯右值引用。要万能引用，`f` 必须自己开模板：`template<class U> void f(U&&)`，`U` 在调用处推导。

6. `auto&&` 是万能引用（`auto` 推导等价于模板推导，Item 2 实测验证）。`for (auto&& e : v)` 绑容器元素的引用，零拷贝、且能正确处理代理元素（`vector<bool>::reference`）；`for (auto e : v)` 逐元素拷贝——遍历订单簿时后者是每 tick 全簿深拷贝的隐形开销。

</details>

---

## 参考与延伸

- 下一节：[Item 25 move vs forward 使用](item25-move-vs-forward-usage.md)
- 回到：[第 5 章](README.md)
