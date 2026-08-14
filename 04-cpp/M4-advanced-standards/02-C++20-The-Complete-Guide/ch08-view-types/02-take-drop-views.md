# take / drop / take_while / drop_while

## take / drop

```cpp
std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// take：取前 N 个
auto first5 = v | std::views::take(5);  // 1,2,3,4,5

// drop：跳过前 N 个
auto after5 = v | std::views::drop(5);  // 6,7,8,9,10

// 配合无限序列
auto top10 = std::views::iota(1) | std::views::take(10);  // 1..10
```

## take_while / drop_while

```cpp
// take_while：取直到条件不满足
auto pos = v | std::views::take_while([](int x) { return x < 5; });
// 1,2,3,4（遇到 5 停止）

// drop_while：跳过直到条件不满足
auto rest = v | std::views::drop_while([](int x) { return x < 5; });
// 5,6,7,8,9,10
```

## take 的特殊用法

```cpp
// take(0)：空范围
auto empty = v | std::views::take(0);

// take(N) where N > size：取全部
auto all = v | std::views::take(100);  // 取全部（不越界）

// 无限序列 + take = 有限序列
auto naturals = std::views::iota(1);  // 1,2,3,...
auto first100 = naturals | std::views::take(100);  // 1..100

// 无限序列 + take_while
auto fib = /* 无限斐波那契序列 */;
auto small_fib = fib | std::views::take_while([](int x) { return x < 1000; });
```

## 组合使用

```cpp
// 分页：每页 10 条，取第 3 页
auto page3 = data
    | std::views::drop(20)  // 跳过前 2 页
    | std::views::take(10); // 取 10 条

// 去掉头尾
auto middle = v
    | std::views::drop(1)    // 去头
    | std::views::reverse
    | std::views::drop(1)    // 去尾
    | std::views::reverse;
```

## 自测题

1. `take(N)` 和 `drop(N)` 的区别？
2. `take_while` 和 `drop_while` 的区别？
3. 无限序列 + `take` 能产生有限序列吗？
4. 如何用 `drop` + `take` 实现分页？
5. `take(100)` 在 5 元素容器上会发生什么？
