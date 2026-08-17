# 视图与管道

## 视图：惰性求值

```cpp
#include <ranges>
#include <algorithm>

std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// C++17：要中间容器
std::vector<int> evens;
std::copy_if(v.begin(), v.end(), std::back_inserter(evens),
             [](int x) { return x % 2 == 0; });
std::vector<int> squares;
std::transform(evens.begin(), evens.end(), std::back_inserter(squares),
               [](int x) { return x * x; });
// 两次遍历 + 两个临时容器

// C++20 Ranges：管道组合，无中间容器
auto result = v
    | std::views::filter([](int x) { return x % 2 == 0; })
    | std::views::transform([](int x) { return x * x; });

for (int x : result) {
    std::cout << x << ' ';  // 4 16 36 64 100
}
// 一次遍历，零临时容器
```

## 管道操作符 |

```cpp
// | 把范围传给视图适配器
v | std::views::filter(pred)     // 过滤
  | std::views::transform(f)     // 变换
  | std::views::take(5)          // 取前 5 个
  | std::views::reverse          // 反转

// 等价写法（不用管道）
std::views::reverse(
    std::views::take(
        std::views::transform(
            std::views::filter(v, pred), f), 5))
// 管道写法更清晰
```

## 视图特性

```cpp
// 1. 惰性：不立即求值
auto view = v | std::views::filter(is_prime);
// 此时没有计算——遍历时才逐元素判断

// 2. 零拷贝：不拥有数据
auto view2 = v | std::views::take(5);
// view2 不拷贝 v 的数据，只是引用

// 3. 可组合
auto view3 = v | std::views::filter(p) | std::views::transform(f) | std::views::take(10);

// 4. 轻量：视图对象通常只有几个指针/迭代器
sizeof(std::views::take_view<std::vector<int>&>);  // 很小
```

## 常用视图

```cpp
// filter：过滤
auto evens = v | std::views::filter([](int x) { return x % 2 == 0; });

// transform：变换
auto squares = v | std::views::transform([](int x) { return x * x; });

// take：取前 N 个
auto first5 = v | std::views::take(5);

// drop：跳过前 N 个
auto after5 = v | std::views::drop(5);

// reverse：反转
auto rev = v | std::views::reverse;

// take_while / drop_while：条件取/跳
auto pos = v | std::views::take_while([](int x) { return x > 0; });

// iota：生成序列
auto seq = std::views::iota(1, 10);  // 1,2,...,9

// keys / values：map 的键/值
auto keys = map | std::views::keys;
auto vals = map | std::views::values;
```

## HFT 应用

```cpp
// 提取所有 AAPL 订单的价格
std::vector<Order> orders;
auto aapl_prices = orders
    | std::views::filter([](const Order& o) { return o.sym == "AAPL"; })
    | std::views::transform(&Order::price);

// 统计前 10 笔成交的总量
auto first10_qty = trades
    | std::views::take(10)
    | std::views::transform(&Trade::qty);
int total = std::accumulate(first10_qty.begin(), first10_qty.end(), 0);
```

## 自测题

1. 视图的四个特性是什么？
2. 管道操作符 `|` 如何工作？
3. `filter` 和 `transform` 的区别？
4. 视图会拷贝数据吗？为什么说"零拷贝"？
5. HFT 中如何用管道提取特定合约的订单价格？
