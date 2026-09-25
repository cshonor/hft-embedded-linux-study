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

<details>
<summary>参考答案</summary>

1. `take(N)`：保留**前 N 个**元素；`drop(N)`：**跳过**前 N 个元素，保留剩下的。
两者互补：`v | drop(k) | take(n)` 就是"从第 k 个开始取 n 个"。当 N 超过实际长度时，`take` 取全部、`drop` 得到空——都不越界。
2. `take_while(pred)`：从头开始**取**，遇到第一个不满足 `pred` 的元素就**停止**（该元素及其之后都不在结果里）。
`drop_while(pred)`：从头开始**跳过**满足 `pred` 的元素，遇到第一个不满足的**保留它及其之后全部**。
```cpp
v = {1,2,3,4,5,6,7,8,9,10};
v | views::take_while([](int x){ return x < 5; });  // 1,2,3,4
v | views::drop_while([](int x){ return x < 5; });  // 5,6,7,8,9,10
```
注意两者都是**惰性**的，`take_while` 遇到失败就停止拉取，不会扫完整个序列。
3. 能。`std::views::iota(1)` 是无界（无限）视图，`| std::views::take(100)` 之后就是一个长度 100 的**有限**视图。
因为视图是惰性的，`take` 只会向前拉取 100 个元素就停止，不会去"展开"那个无限序列：
```cpp
auto first100 = std::views::iota(1) | std::views::take(100);   // 1..100
```
同理 `iota(1) | take_while(pred)` 也能在条件首次不成立时终止。
4. 先 `drop` 跳过前面的页，再 `take` 取一页的量：
```cpp
constexpr std::size_t page_size = 10;
std::size_t page = 2;                       // 第 3 页（从 0 开始）
auto page3 = data
    | std::views::drop(page * page_size)
    | std::views::take(page_size);
```
因为 `drop` / `take` 对越界都是安全的，最后一页不足 `page_size` 时也不会出错。
5. 什么都不会出错——它**只取到容器末尾**，`take` 的实际长度是 `min(N, size)`，即 5 个元素全部保留，不会越界、不会补默认值。
`views::take(0)` 则得到**空视图**。这也是 `take` 与 `std::vector::resize` 之类"必须凑够 N 个"的操作的区别。

</details>
