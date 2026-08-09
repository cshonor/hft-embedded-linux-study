# 7.3 可配对（Composable）仿函数
> 第 7 章 仿函数 · 第 3 节 · 上一节：[7.2 内置仿函数](02-builtin-functors.md) · 下一节：[第 8 章 适配器](../ch08-adapters/README.md)

## 为什么要学这个（先建立直觉）

C 里函数组合完全靠手写——没有"把两个函数组合成新函数"的机制：

```c
// C: 手动组合
int greater_than_5(int x) { return x > 5; }
int not_greater_than_5(int x) { return !greater_than_5(x); }
// 每种组合都要手写一个新函数
```

C++03 的仿函数配对机制允许用适配器组合：

```cpp
// C++03: 用 bind2nd + not1 组合
std::not1(std::bind2nd(std::less<int>(), 5))
// 等价于 !(x < 5) = (x >= 5)
// 组合出一个新的谓词，无需手写
```

C++11 后 lambda 让组合更直观：

```cpp
// C++11: lambda 直接写
auto pred = [](int x) { return x >= 5; };
// 或用 not_fn（C++17）
auto neg = std::not_fn(pred);  // !(x >= 5)
```

理解配对机制的历史，你才能理解为什么 `bind1st`/`bind2nd`/`not1`/`not2` 存在，以及为什么它们被淘汰。

## 这节讲什么

继承 `unary_function`/`binary_function` 的仿函数可以被适配器包装成新仿函数——这就是"可配对"。

### 配对机制

```
原仿函数 + 适配器 = 新仿函数

less<int>()  +  bind2nd(_, 5)  →  x < 5（一元谓词）
x < 5        +  not1(_)         →  !(x < 5) = x >= 5（否定谓词）
```

### bind1st / bind2nd

```cpp
// bind2nd: 固定第二个参数
std::bind2nd(std::less<int>(), 5)
// 生成: [x](x) { return x < 5; }

// bind1st: 固定第一个参数
std::bind1st(std::less<int>(), 5)
// 生成: [x](x) { return 5 < x; }  即 x > 5
```

源码实现（简化）：
```cpp
template<class Operation>
class binder2nd : public unary_function<
    typename Operation::first_argument_type,
    typename Operation::result_type>
{
protected:
    Operation op;
    typename Operation::second_argument_type value;
public:
    binder2nd(const Operation& x,
              const typename Operation::second_argument_type& y)
        : op(x), value(y) {}
    result_type operator()(const argument_type& x) const {
        return op(x, value);  // 固定第二个参数
    }
};

template<class Operation, class T>
binder2nd<Operation> bind2nd(const Operation& op, const T& x) {
    return binder2nd<Operation>(op, x);
}
```

### not1 / not2

```cpp
// not1: 否定一元谓词
std::not1(std::bind2nd(std::less<int>(), 5))
// 生成: !(x < 5) = x >= 5

// not2: 否定二元谓词
std::not2(std::less<int>())
// 生成: !(a < b) = a >= b
```

### 组合链示例

```cpp
// C++03: 找第一个 >= 5 的元素
std::vector<int> v = {1, 3, 5, 7, 9};
auto it = std::find_if(v.begin(), v.end(),
    std::not1(std::bind2nd(std::less<int>(), 5)));
// !(x < 5) = x >= 5 → 找到 5

// C++11: lambda
auto it = std::find_if(v.begin(), v.end(),
    [](int x) { return x >= 5; });

// C++17: not_fn
auto it = std::find_if(v.begin(), v.end(),
    std::not_fn([](int x) { return x < 5; }));
```

### C++11 后的替代

| C++03 适配器 | C++11+ 替代 | 说明 |
|-------------|-----------|------|
| `bind1st(op, val)` | `std::bind(op, val, _1)` | 固定第一个参数 |
| `bind2nd(op, val)` | `std::bind(op, _1, val)` | 固定第二个参数 |
| `not1(pred)` | `!pred` 或 `std::not_fn(pred)` (C++17) | 否定一元 |
| `not2(pred)` | `std::not_fn(pred)` (C++17) | 否定二元 |
| `mem_fun`/`mem_fun_ref` | lambda 或 `std::mem_fn` | 成员函数包装 |
| `ptr_fun` | 不需要 | 函数指针→仿函数 |

## 常见错误（新手踩坑）

### 错误 1：配对链太长可读性差

```cpp
// ❌ C++03 配对链可读性极差
auto it = std::find_if(v.begin(), v.end(),
    std::not1(
        std::bind2nd(
            std::less_equal<int>(), 10)));
// !(x <= 10) = x > 10 —— 谁看得懂？

// ✅ lambda 直接写
auto it = std::find_if(v.begin(), v.end(),
    [](int x) { return x > 10; });
```

### 错误 2：C++11 后还在用 bind2nd

```cpp
// ❌ C++11 后 bind2nd 已废弃
std::bind2nd(std::plus<int>(), 5);  // C++11 deprecated, C++17 removed

// ✅ std::bind
auto add5 = std::bind(std::plus<int>(), std::placeholders::_1, 5);

// ✅✅ lambda（最简洁）
auto add5 = [](int x) { return x + 5; };
```

### 错误 3：bind 的占位符混淆

```cpp
// std::bind 的占位符 _1, _2 表示调用时传入的参数位置
auto f = std::bind(std::less<int>(), std::placeholders::_1, 5);
f(3);   // 3 < 5 → true（_1 = 3）

auto g = std::bind(std::less<int>(), 5, std::placeholders::_1);
g(3);   // 5 < 3 → false（_1 = 3）
// bind1st(op, 5) 等价于 bind(op, 5, _1)
// bind2nd(op, 5) 等价于 bind(op, _1, 5)
```

## 新手要点（和 C 的区别）

| C | C++ | 区别 |
|----|-----|------|
| 手写组合函数 | 适配器自动组合 | C++ 可组合 |
| 无状态携带 | bind 捕获参数 | C++ 更灵活 |
| 无类型推导 | 模板自动推导 | C++ 类型安全 |
| 无内联 | 仿函数可内联 | C++ 更快 |
| N/A | lambda 更简洁 | C++11 首选 |

## HFT 关联

- **新代码用 lambda**：HFT 新代码一律用 lambda 组合谓词，可读 + 可内联
- **理解 bind 读老代码**：量化交易系统老代码可能有 `bind2nd`/`not1` 链，理解才能维护
- **`std::function` 有开销**：不要用 `std::function` 做组合——类型擦除 + 可能堆分配，热路径避免
- **编译期组合**：lambda/仿函数组合是编译期的（类型唯一），零运行时开销

## 代码自测

### Q1: bind2nd(less<int>(), 5) 生成什么？

```cpp
auto pred = std::bind2nd(std::less<int>(), 5);
pred(3);  // ?
pred(7);  // ?
```
> pred(3) 和 pred(7) 分别返回什么？

<details>
<summary>答案与复习指引</summary>

- `pred(3)` = `true`（3 < 5）
- `pred(7)` = `false`（7 < 5 不成立）

`bind2nd(less<int>(), 5)` 把 `less<int>` 的第二个参数固定为 5，生成一个一元谓词 `x < 5`。

等价 lambda：
```cpp
auto pred = [](int x) { return x < 5; };
```

**复习：** → [bind1st / bind2nd](./03-composable-functors.md)
</details>

### Q2: not1(bind2nd(less<int>(), 5)) 等价于什么？

```cpp
auto pred = std::not1(std::bind2nd(std::less<int>(), 5));
pred(3);  // ?
pred(7);  // ?
```
> 写出等价的 lambda。

<details>
<summary>答案与复习指引</summary>

- `pred(3)` = `false`（`!(3 < 5)` = `!true` = `false`）
- `pred(7)` = `true`（`!(7 < 5)` = `!false` = `true`）

等价 lambda：
```cpp
auto pred = [](int x) { return !(x < 5); };  // 即 x >= 5
// 或更直接
auto pred = [](int x) { return x >= 5; };
```

**组合链**：
1. `less<int>()` → 二元：`a < b`
2. `bind2nd(_, 5)` → 一元：`x < 5`
3. `not1(_)` → 一元否定：`!(x < 5)` = `x >= 5`

**教训**：C++03 的组合链 `not1(bind2nd(less<int>(), 5))` 极其难读，lambda 直接写 `x >= 5` 清晰百倍。

**复习：** → [组合链示例](./03-composable-functors.md)
</details>

### Q3: std::bind 和 lambda 哪个更好？

```cpp
// 方式 A: std::bind
auto add5_bind = std::bind(std::plus<int>(), std::placeholders::_1, 5);

// 方式 B: lambda
auto add5_lambda = [](int x) { return x + 5; };
```
> 两者有什么区别？哪个更推荐？

<details>
<summary>答案与复习指引</summary>

| 特性 | `std::bind` | lambda |
|------|-----------|--------|
| 可读性 | 差（占位符混淆） | 好（直接写逻辑） |
| 性能 | 可能堆分配 | 零开销（闭包类型已知） |
| 过载 | 难处理重载函数 | 无此问题 |
| 调试 | 难（嵌套类型名复杂） | 易（可直接断点） |
| 灵活性 | 可绑定任意参数位置 | 需要手动写逻辑 |

**推荐**：C++14+ 一律用 lambda。`std::bind` 只在需要"延迟绑定"（运行时决定参数）时才有价值。

**Effective Modern C++ Item 34**：优先用 lambda 而非 `std::bind`。

**HFT**：lambda 可内联、零开销、可读——热路径唯一选择。

**复习：** → [C++11 后的替代](./03-composable-functors.md)
</details>

### Q4: not_fn 和 not1 有什么区别？

```cpp
// C++03: not1 需要仿函数有 argument_type
struct Pred : std::unary_function<int, bool> {
    bool operator()(int x) const { return x > 5; }
};
std::not1(Pred{});  // OK

// C++17: not_fn 对任何可调用对象工作
std::not_fn([](int x) { return x > 5; });  // OK
```
> 为什么 not_fn 能工作而 not1 不能接受 lambda？

<details>
<summary>答案与复习指引</summary>

**not1** 需要从仿函数萃取 `argument_type` typedef 来生成否定版。lambda 闭包类型没有这个 typedef，所以 `not1` 编译失败。

**not_fn**（C++17）用完美转发和 `decltype` 推导参数类型，不需要 `argument_type`。它对任何可调用对象（lambda、函数指针、仿函数、`std::function`）都能工作。

**not_fn 实现原理**（简化）：
```cpp
template<class F>
class _Not_fn {
    F f;
public:
    template<class... Args>
    auto operator()(Args&&... args) const
        -> decltype(!std::invoke(f, std::forward<Args>(args)...))
    {
        return !std::invoke(f, std::forward<Args>(args)...);
    }
};

template<class F>
_Not_fn<std::decay_t<F>> not_fn(F&& f) {
    return _Not_fn<std::decay_t<F>>(std::forward<F>(f));
}
```

**教训**：C++17+ 用 `not_fn` 替代 `not1`/`not2`，或直接写否定 lambda。

**复习：** → [C++11 后的替代](./03-composable-functors.md)
</details>

## 参考与延伸

- 上一节：[7.2 内置仿函数](02-builtin-functors.md)
- 下一节：[第 8 章 适配器](../ch08-adapters/README.md)
- 参考：Effective Modern C++ Item 34（优先 lambda 而非 bind）
