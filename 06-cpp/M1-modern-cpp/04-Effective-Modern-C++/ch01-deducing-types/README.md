# 第 1 章 类型推导

**Deducing Types** — Items 1–4

## 本章讲什么

C++11 引入了 `auto` 和模板类型推导，C++14 又让 `decltype` 参与返回值推导。类型推导让你少敲键盘，但也让代码里"看不见的类型"变多——尤其当一个表达式经过 `auto` + 模板 + `decltype` 三层推导后，最终类型可能和你直觉完全相反。本章把三种推导机制的**规则边界**讲透，这是读 Modern C++ 代码（含 DPDK C++ 封装、muduo 回调）的前提。

---

## 各 Item 要点

### Item 1：理解模板类型推导

模板形如 `template<class T> void f(ParamType param)`，调用 `f(expr)` 时推导分两步：按 `expr` 推导 `T`，再由 `ParamType` 形态决定最终类型。三种 `ParamType` 形态：

| ParamType 形态 | 推导规则要点 | 典型陷阱 |
|----------------|-------------|----------|
| `T&`（引用） | 忽略 `expr` 的引用性，`T` 推为去引用后的类型 | 数组/函数实参仍退化 |
| `T&&`（万能引用） | 左值实参 → `T` 推为 `T&`；右值 → `T` | 这是"完美转发"的根基 |
| `T`（按值） | 忽略引用、const、volatile；指针/数组退化为指针 | 顶层 const 被丢弃，底层 const 保留 |

**关键直觉**：按值传递会**剥掉顶层 const**——传 `const int ci` 给 `T` 形参，`T` 是 `int` 不是 `const int`。这是很多人在泛型代码里"const 怎么没了"的根因。

**数组/函数实参**：除非 `ParamType` 是引用，否则数组退化为指针、函数退化为函数指针。这个退化规则和 C 完全一致（见《C 和指针》ch08）。

### Item 2：理解 auto 类型推导

`auto` 推导**几乎等同**模板推导——把 `auto` 当 `T`、把声明修饰当 `ParamType`，套 Item 1 三规则。但有一个**例外**：

```cpp
auto x = {1, 2, 3};   // auto 推为 std::initializer_list<int>，不是 int！
template<class T> void f(T); f({1,2,3});  // 编译失败：模板不推导 braced-init-list
```

`auto` 接大括号初始化会推成 `initializer_list`，而等价的模板调用直接报错。这个不一致是 `auto` 和模板推导**唯一的**语义差别，但它是真实代码里最常见的"为什么编译不过"来源。

### Item 3：理解 decltype

`decltype(expr)` 绝大多数时候返回 `expr` 的确切类型，不做模板/auto 那套退化。但它有一个反直觉特例：

```cpp
int x = 0;          // decltype(x)   → int
decltype((x)) y;    // decltype((x)) → int&  ！加括号变成引用
```

**`decltype(变量名)` 得类型，`decltype((变量名))` 得引用**——多一层括号，语义突变。这影响 `decltype(auto)` 返回值推导（C++14）：`decltype(auto) f() { return (x); }` 会返回 `x` 的引用，可能悬垂。

### Item 4：如何查看推导结果

三种手段，由"编译期 → 运行时"递进：

1. **编译期诊断**：`template<class T> class TD;` 然后 `TD<decltype(x)> xType;`——编译器报错里会打印类型。最轻量，无需运行。
2. **运行时 RTTI**：`typeid(x).name()`——但名字是编译器修饰名（GCC 会输出 `i`、`PKc` 这种），且对引用/顶 const 会"撒谎"（按值传参后退化）。
3. **Boost.TypeIndex**：`boost::typeindex::type_id_with_cvr<T>()` 能保留 const/volatile/引用，是最精确的运行时手段。

**HFT 实践**：调试模板推导首选方法 1（零运行开销，编译期就暴露类型）；方法 2 只在确认 ABI / 对象布局时偶尔用，且要手动解 mangled name。

---

## HFT 关联

- **万能引用 + 完美转发**是 muduo / DPDK C++ 封装里回调注册的基石：`template<class F> void set_cb(F&& f)` 用万能引用避免不必要的拷贝。不理解 Item 1 的第三种形态，读不懂这类接口。
- **`auto` 与代理类型**（Item 6 会详谈）：`auto p = vp.lock()` 拿到 `shared_ptr` 是值语义；但 `auto b = vector<bool>[0]` 拿到的是**代理对象** `vector<bool>::reference`，不是 `bool`——这类隐式类型在热路径里可能引入意外的间接寻址。推导规则决定了你拿到什么。
- **`decltype` 用于 SFINAE / `enable_if`**：模板元编程里用 `decltype` 探测表达式是否合法（`void_t` 技巧），是编译期分支的前提，HFT 用它做"编译期策略选择"替代运行时虚函数。

---

## 自测题

1. `template<class T> void f(T param);` 调用 `f(ci)`（`ci` 是 `const int`），`T` 推导成什么？为什么顶层 const 消失？
2. `auto x = {1,2,3};` 和 `auto x{1};`（C++17 前/后）分别推导出什么？模板函数能推导 `{1,2,3}` 吗？
3. `decltype(x)` 和 `decltype((x))` 的区别是什么？这对 `decltype(auto)` 返回值有什么影响？
4. 为什么 `typeid(x).name()` 对引用和顶层 const 会"撒谎"？想精确保留 cv 限定该用什么？
5. 万能引用 `T&&` 接收左值时 `T` 推导成什么？这个机制为什么是完美转发的根基？
