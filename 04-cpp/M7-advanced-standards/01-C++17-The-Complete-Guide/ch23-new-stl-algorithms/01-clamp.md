# std::clamp

## 基本用法

```cpp
#include <algorithm>

// clamp(val, lo, hi)：将 val 限制在 [lo, hi] 范围内
int x = std::clamp(15, 0, 10);   // 10（超出上界，返回上界）
int y = std::clamp(-5, 0, 10);   // 0（超出下界，返回下界）
int z = std::clamp(5, 0, 10);    // 5（在范围内，原值返回）

// 等价于 std::max(lo, std::min(val, hi))
// 但更清晰、不易写错
```

## 带比较器版本

```cpp
// 自定义比较：cmp(a, b) 返回 a < b
struct Price {
    double value;
    int tick;  // 最小变动价位
};

auto cmp = [](const Price& a, const Price& b) {
    return a.value < b.value;
};

Price p{100.5, 1};
Price lo{99.0, 1};
Price hi{101.0, 1};

Price clamped = std::clamp(p, lo, hi, cmp);
```

## 要求

- `lo` 必须不大于 `hi`（否则未定义行为）
- 比较器必须满足严格弱序
- 返回值是 `val`、`lo` 或 `hi` 的 const 引用

## C++17 前的替代

```cpp
// C++14 手写
int x = std::max(lo, std::min(val, hi));
// 问题：参数顺序容易写反（max/min 谁在外？）

// C++17 clamp
int x = std::clamp(val, lo, hi);
// 清晰、不易错
```

## HFT 应用

```cpp
// 限制价格滑点
double fill_px = std::clamp(order_px, best_bid, best_ask);

// 限制数量
int safe_qty = std::clamp(requested_qty, 0, max_qty);

// 限制仓位
double safe_pos = std::clamp(current_pos + delta, -max_pos, max_pos);
```

## 自测题

1. `clamp(val, lo, hi)` 等价于什么表达式？
2. `lo > hi` 时会怎样？
3. 带比较器的 clamp 怎么用？比较器要满足什么条件？
4. 为什么说 clamp 比手写 `max(lo, min(val, hi))` 好？
5. HFT 中限制价格滑点的 clamp 写法？

<details>
<summary>参考答案</summary>

1. `std::clamp(v, lo, hi)` 返回 `v < lo ? lo : (hi < v ? hi : v)`，即等价于 **`std::max(lo, std::min(v, hi))`**。
注意它返回的是 `const T&`（引用）：`v` 落在区间内时直接返回 `v` 本身，不做拷贝；三个参数都按 `const T&` 传入且各只求值一次。
2. `lo > hi` 违反了标准规定的前置条件（`lo <= hi`），按 C++17 属于**未定义行为**，不要指望它返回有意义的值。
另外若 `v` 是 NaN，它与 `lo`、`hi` 的比较都为假，会原样返回 `v`——这也不符合「夹在区间内」的语义。调用前应自行保证 `lo <= hi` 且值非 NaN。
3. 带比较器的重载是 `std::clamp(v, lo, hi, comp)`，`comp` 必须满足**严格弱序**（strict weak ordering：非自反、可传递、等价关系可传递），语义上表示「小于」。
求值方式为 `comp(v, lo) ? lo : comp(hi, v) ? hi : v`：
```cpp
Price clamped = std::clamp(p, lo, hi,
    [](const Price& a, const Price& b) { return a.value < b.value; });
```
自定义类型要么提供 `operator<`，要么显式传比较器。
4. `std::max(lo, std::min(val, hi))` 里 `min` 和 `max` 谁在外层很容易写反（写反变成 `min(lo, max(val, hi))`，结果完全错误），读起来还要在脑子里绕一圈。
`clamp(val, lo, hi)` 参数顺序直观（值、下界、上界），语义一目了然；而且它是 `constexpr`、各参数只求值一次、返回引用避免拷贝，边界语义由标准库保证。
5. 把成交价夹在买一/卖一之间，防止滑点超出盘口：
```cpp
double fill_px = std::clamp(order_px, best_bid, best_ask);
int    safe_qty = std::clamp(requested_qty, 0, max_qty);
double safe_pos = std::clamp(current_pos + delta, -max_pos, max_pos);
```
使用前应确保 `best_bid <= best_ask`（盘口正常时成立），否则触发未定义行为。

</details>
