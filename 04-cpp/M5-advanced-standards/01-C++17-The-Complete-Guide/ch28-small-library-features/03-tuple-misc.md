# <tuple> 改进与杂项

## tuple_cat：合并 tuple

```cpp
#include <tuple>

auto t1 = std::make_tuple(1, 2.0);
auto t2 = std::make_tuple("hello", 'c');

auto merged = std::tuple_cat(t1, t2);
// merged = tuple<int, double, const char*, char>(1, 2.0, "hello", 'c')

// 合并多个
auto t3 = std::make_tuple(true);
auto big = std::tuple_cat(t1, t2, t3);
// tuple<int, double, const char*, char, bool>

// 合并 pair
auto p = std::make_pair(42, 3.14);
auto from_pair = std::tuple_cat(t1, p);
// tuple<int, double, int, double>(1, 2.0, 42, 3.14)
```

## apply 与 tuple 配合（见第 25 章）

```cpp
// apply 展开 tuple 调用函数
auto args = std::make_tuple(1, 2.0, "hi");
std::apply(some_function, args);
```

## make_from_tuple

```cpp
// 用 tuple 构造对象
struct Config {
    Config(int id, double alpha, std::string name);
};

auto params = std::make_tuple(1, 0.05, "strategy_1");
Config cfg = std::make_from_tuple<Config>(params);
// 等价 Config(1, 0.05, "strategy_1")
```

## <cmath> 新增

```cpp
#include <cmath>

// hypot 三参数版
std::hypot(3.0, 4.0);       // 5.0（已有）
std::hypot(3.0, 4.0, 12.0); // 13.0（C++17 新增）

// 特殊数学函数（C++17 部分引入）
// std::lerp(a, b, t) — 线性插值（C++20 正式）
// std::beta, std::erf, std::legendre 等特殊函数
```

## variant/optional/any 辅助

```cpp
#include <variant>
#include <optional>

// swap
std::optional<int> a = 1, b = 2;
std::swap(a, b);  // C++17 swap 特化

// hash 支持
std::variant<int, std::string> v = "hello";
auto h = std::hash<decltype(v)>{}(v);  // C++17 可哈希

// variant 的 npos
if (v.index() != std::variant_npos) {
    // variant 有值
}
```

## 实际应用

```cpp
// 1. 多来源参数合并
auto default_params = std::make_tuple(0.05, 100, true);
auto runtime_params = std::make_tuple(get_dynamic_alpha());
auto all_params = std::tuple_cat(default_params, runtime_params);
auto strategy = std::make_from_tuple<Strategy>(all_params);

// 2. 三维距离计算
double dist = std::hypot(x2-x1, y2-y1, z2-z1);
```

## 自测题

1. `std::tuple_cat` 做什么？能合并 `pair` 吗？
2. `make_from_tuple` 和 `apply` 的区别？
3. `std::hypot` 三参数版计算什么？
4. `std::variant` 的 `hash` 支持是 C++17 加的吗？
5. 多来源策略参数如何用 `tuple_cat` 合并后构造对象？
