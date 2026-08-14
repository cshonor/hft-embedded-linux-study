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
