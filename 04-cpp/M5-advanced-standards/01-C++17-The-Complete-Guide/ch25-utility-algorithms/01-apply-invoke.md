# std::apply 与 std::invoke

## std::apply：tuple 展开为函数参数

```cpp
#include <tuple>

void foo(int a, double b, const std::string& c);

auto args = std::make_tuple(1, 2.0, "hello"s);
std::apply(foo, args);   // 等价 foo(1, 2.0, "hello")
```

**原理**：`apply` 用 `tuple_size` + `get<I>` 在编译期展开 tuple 的每个元素，作为函数参数传入。

## std::invoke：统一调用语法

```cpp
#include <functional>

struct Obj {
    int x;
    void show() { std::cout << x; }
};

Obj o{42};
Obj* p = &o;

// 普通函数
void f(int);
std::invoke(f, 42);

// 成员函数指针
std::invoke(&Obj::show, o);       // (o.*&Obj::show)()
std::invoke(&Obj::show, p);       // (p->*&Obj::show)()

// 成员指针（数据成员）
std::cout << std::invoke(&Obj::x, o);  // o.x
std::cout << std::invoke(&Obj::x, p);  // p->x

// 函数对象
auto lam = [](int x) { return x * 2; };
std::invoke(lam, 21);  // 42
```

**解决的问题**：成员函数指针和成员指针的调用语法不一致：
- `(obj.*pmf)(args...)` vs `(ptr->*pmf)(args...)`
- `obj.*pmd` vs `ptr->*pmd`
`invoke` 统一了所有调用形式。

## apply + invoke 的关系

```cpp
// apply 内部用 invoke 调用
// 等价于：
template <typename F, typename Tuple, size_t... I>
auto apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>) {
    return std::invoke(std::forward<F>(f), std::get<I>(std::forward<Tuple>(t))...);
}

template <typename F, typename Tuple>
auto apply(F&& f, Tuple&& t) {
    return apply_impl(
        std::forward<F>(f), std::forward<Tuple>(t),
        std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>{}
    );
}
```

## 实际应用

```cpp
// 1. 策略参数展开
struct Strategy {
    Strategy(double alpha, int period, bool use_filter);
};

auto params = std::make_tuple(0.05, 20, true);
auto strat = std::make_from_tuple<Strategy>(params);

// 2. 消息分发：从收到的 tuple 构造参数调用 handler
void handle_order(int sym_id, double price, int qty);

auto msg = std::make_tuple(1, 100.5, 200);
std::apply(handle_order, msg);

// 3. 配合 bind/lambda
auto add = [](int a, int b) { return a + b; };
auto args = std::make_pair(3, 4);
std::cout << std::apply(add, args);  // 7
```

## 自测题

1. `std::apply` 和 `std::invoke` 各解决什么问题？
2. 成员函数指针不用 `invoke` 怎么调用？语法复杂在哪？
3. `apply` 内部是怎么展开 tuple 的？（提示：`index_sequence`）
4. `invoke` 能调用成员数据指针吗？怎么用？
5. 消息分发场景如何用 `apply` 展开参数？

<details>
<summary>参考答案</summary>

1. `std::apply` 解决的是「**tuple 展开成函数实参**」的问题：把 `std::tuple`/`std::pair`/`std::array` 里的每个元素按顺序取出，作为位置参数调用一个可调用物。
`std::invoke` 解决的是「**统一调用语法**」的问题：用同一个写法调用普通函数、函数对象/lambda、成员函数指针、数据成员指针（即标准里的 INVOKE 语义）。
两者正交——而且 `apply` 内部就是靠 `invoke` 完成调用的，所以 `apply` 也能直接驱动成员函数指针。
2. 不用 `invoke` 就得用内建的「指向成员」运算符，而且对象形式是值/引用还是指针，写法还不一样：
```cpp
std::invoke(&Obj::show, o);   // 等价 (o.*&Obj::show)()
std::invoke(&Obj::show, p);   // 等价 (p->*&Obj::show)()
```
复杂点在于：`.*` 和 `->*` 是两个不同的运算符，括号还容易漏（`(o.*pmf)()` 少了括号会被解析错），泛型代码里必须靠 `if constexpr` 或重载分别处理对象、指针、`reference_wrapper`、智能指针等各种情况。`invoke` 把这些差异全部收进标准库，泛型代码只写一种形式。
3. 靠 `std::index_sequence` 在编译期把下标包展开成 `std::get<I>(t)...`：
```cpp
template <typename F, typename Tuple, std::size_t... I>
auto apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>) {
    return std::invoke(std::forward<F>(f),
                       std::get<I>(std::forward<Tuple>(t))...);
}

template <typename F, typename Tuple>
auto apply(F&& f, Tuple&& t) {
    return apply_impl(std::forward<F>(f), std::forward<Tuple>(t),
        std::make_index_sequence<
            std::tuple_size_v<std::decay_t<Tuple>>>{});
}
```
`tuple_size_v` 拿到元素个数，`make_index_sequence` 生成 `0..N-1` 的编译期下标包，一次包展开就完成了 tuple 到实参列表的转换。
4. 可以。数据成员指针同样适用 INVOKE 语义，第一个「参数」是对象（值、引用、指针、`reference_wrapper`、智能指针均可）：
```cpp
Obj o{42};
Obj* p = &o;
std::cout << std::invoke(&Obj::x, o);  // o.x   → 42
std::cout << std::invoke(&Obj::x, p);  // p->x  → 42
```
这在泛型代码里很有用：同一个模板既能取成员函数的调用结果，也能取成员变量的值，不必为数据成员单独写分支。
5. 把收到的消息先组织成 tuple，再用 `apply` 一次性展开成 handler 的实参：
```cpp
void handle_order(int sym_id, double price, int qty);

auto msg = std::make_tuple(1, 100.5, 200);
std::apply(handle_order, msg);        // 等价 handle_order(1, 100.5, 200)

// 配合 lambda / bind 也可以
auto add = [](int a, int b) { return a + b; };
std::cout << std::apply(add, std::make_pair(3, 4));  // 7
```
这样「解析层产出 tuple」与「业务层签名」解耦，加字段只改 tuple 和 handler 签名，不用改分发代码。

</details>
