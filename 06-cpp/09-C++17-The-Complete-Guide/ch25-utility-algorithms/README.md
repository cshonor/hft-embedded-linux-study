# 第 25 章 其他工具函数与算法

**Other Utility Functions and Algorithms**

## 本章讲什么

C++17 杂项工具：`std::apply`、`std::make_from_tuple`、`std::as_const`、`std::clamp`（见第 23 章）、`std::sample`、`std::launder`（见第 32 章）。

## 要点

### `std::apply`：用 tuple 调用函数

```cpp
#include <tuple>

void foo(int a, double b, const std::string& c);

auto args = std::make_tuple(1, 2.0, "hello");
std::apply(foo, args);   // 等价 foo(1, 2.0, "hello")
```

把 tuple 展开成函数参数。元编程中常用——把编译期 tuple 转成运行期调用。

### `std::make_from_tuple`：用 tuple 构造对象

```cpp
struct Obj {
    Obj(int, double, std::string);
};

auto args = std::make_tuple(1, 2.0, "hi");
Obj o = std::make_from_tuple<Obj>(args);   // Obj(1, 2.0, "hi")
```

### `std::as_const`：获得 const 引用

```cpp
std::string s = "hello";
const std::string& cs = std::as_const(s);   // 等价 const_cast<const std::string&>(s)

// 用途：强制走 const 重载
void foo(std::string&);       // 非 const 版
void foo(const std::string&); // const 版
foo(s);              // 调非 const 版
foo(std::as_const(s)); // 调 const 版
```

### `std::sample`：随机采样

```cpp
#include <algorithm>
#include <random>

std::vector<int> pop = {1,2,3,4,5,6,7,8,9,10};
std::vector<int> sample(3);

std::mt19937 rng{seed};
std::sample(pop.begin(), pop.end(),
            sample.begin(), 3, rng);
// sample 中有 3 个随机不重复元素
```

蓄水池抽样（reservoir sampling），O(n) 时间，适合从大集合中均匀采样 k 个。

### `std::clamp`（见第 23 章，此处略）

### `std::data` / `std::size` / `std::empty`

```cpp
int arr[5] = {1,2,3,4,5};
std::size(arr);    // 5（数组版，C++17）
std::data(arr);    // arr（指针）
std::empty(arr);   // false

std::vector<int> v;
std::size(v);      // v.size()
std::data(v);      // v.data()
std::empty(v);     // v.empty()
```

统一了数组和容器的接口，泛型代码不用区分。

## HFT 关联

- **`apply` 调用配置构造**：策略参数存为 tuple，`apply(strategy_factory, params)` 展开
- **`as_const` 强制 const 路径**：风控检查用 `as_const` 确保不误改行情对象，走 const 重载。
- **`sample` 抽样回测**：从历史 tick 中随机抽样 k 个做快速回测验证，蓄水池抽样 O(n)。
- **`size`/`data`/`empty` 泛型**：模板中 `std::size(x)` 对数组和容器都行，不用写 `sizeof(arr)/sizeof(arr[0])`。
- **`make_from_tuple` 工厂构造**：配置反序列化为 tuple 后，`make_from_tuple<Strategy>(cfg_tuple)` 构造策略对象。

## 自测题

1. `std::apply` 和 `std::make_from_tuple` 的区别？
2. `std::as_const` 解决什么问题？
3. `std::sample` 用什么算法？复杂度？
4. `std::size` 对数组和容器分别调用什么？
5. HFT 配置构造如何用 `make_from_tuple`？
