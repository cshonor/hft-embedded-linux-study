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

<details>
<summary>参考答案</summary>

1. 视图（view）的四个关键特性：
   1. **是 range**：能用 `begin`/`end` 遍历。
   2. **轻量**：拷贝、移动、析构都是 **O(1)**（只持有引用/迭代器 + 变换函数，`sizeof` 很小），可随便按值传递。
   3. **惰性（lazy）**：只在被迭代时才计算，不提前跑一遍。
   4. **非拥有（non-owning）**：不持有元素的所有权，只是对底层范围的一层"看法"（因此底层容器必须活得比视图久）。
2. `|` 把左边的范围"喂给"右边的**范围适配器闭包（range adaptor closure）**：`r | c` 等价于 `c(r)`。
`std::views::filter(f)` 这类适配器在只给一个参数时返回一个闭包对象（而不是立即计算），多个闭包还能用 `|` 继续串接（闭包之间也定义了 `|`），于是
```cpp
v | views::filter(f) | views::transform(g)   // 等价 transform(g)(filter(f)(v))
```
写起来像 Unix 管道，读起来是数据从左到右依次流过。
3. `filter(pred)` 按谓词**筛选元素**：保留满足条件的，元素个数可能变少，**元素值本身不变**。
`transform(fn)` 对每个元素做**映射**：元素个数不变，**值被替换成 `fn(elem)` 的结果**。
一句话：`filter` 决定"要哪些"，`transform` 决定"变成什么"。
4. 不拷贝。视图只保存**对底层范围的引用/迭代器**以及变换函数本身，不复制任何元素；遍历时才按需逐个元素计算（惰性）。
所以 `sizeof(view)` 很小、构造是 O(1)、管道组合层数再多也不会产生中间容器——这就是"零拷贝"的含义。
注意：零拷贝也意味着**底层容器必须比视图活得久**（否则迭代器悬垂），且 `filter` 这类适配器要求被适配的范围至少是 `viewable_range`。
5. ```cpp
auto aapl_prices = orders
    | std::views::filter([](const Order& o) { return o.sym == "AAPL"; })
    | std::views::transform(&Order::price);
for (double px : aapl_prices) { /* ... */ }
```
全程不创建中间 `vector<Order>` 或 `vector<double>`：一次遍历、按需取值，热路径外的分析/统计代码因此既简洁又不产生额外分配。

</details>
