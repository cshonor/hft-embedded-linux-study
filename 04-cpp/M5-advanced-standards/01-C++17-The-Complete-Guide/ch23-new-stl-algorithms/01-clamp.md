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
