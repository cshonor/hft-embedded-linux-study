# filter / transform 视图

## filter

```cpp
std::vector<int> v = {1, 2, 3, 4, 5, 6};

// 过滤偶数
auto evens = v | std::views::filter([](int x) { return x % 2 == 0; });
// 2, 4, 6

// filter 保持元素的引用
for (int& x : v | std::views::filter(...)) {
    x *= 2;  // 修改原容器元素
}
```

## transform

```cpp
// 变换
auto squares = v | std::views::transform([](int x) { return x * x; });
// 1, 4, 9, 16, 25, 36

// transform 返回值类型（不能修改原元素）
for (auto x : v | std::views::transform(square)) {
    // x 是 square 的返回值，修改不影响原容器
}
```

## 链式组合

```cpp
// filter → transform → take
auto result = v
    | std::views::filter([](int x) { return x % 2 == 0; })  // 2,4,6
    | std::views::transform([](int x) { return x * x; })     // 4,16,36
    | std::views::take(2);                                     // 4,16

// 执行顺序：从左到右
// 先过滤 → 再变换 → 再取前2个
```

## 成员投影

```cpp
struct Order {
    std::string sym;
    double price;
    int qty;
};
std::vector<Order> orders;

// 提取所有 AAPL 订单的价格
auto prices = orders
    | std::views::filter([](const Order& o) { return o.sym == "AAPL"; })
    | std::views::transform(&Order::price);
// 直接用成员指针做 transform

// 等价于
auto prices2 = orders
    | std::views::filter([](const Order& o) { return o.sym == "AAPL"; })
    | std::views::transform([](const Order& o) { return o.price; });
```

## 自测题

1. `filter` 和 `transform` 的区别？
2. `filter` 视图能修改原容器元素吗？`transform` 呢？
3. 链式组合的执行顺序是什么？
4. 如何用成员指针做 `transform`？
5. 提取特定字段值的管道写法？

<details>
<summary>参考答案</summary>

1. `filter(pred)` 按谓词**筛选**：只保留满足条件的元素，**元素个数可变**，通过的元素本身不变（`v | filter(f)` 的元素就是原元素）。
`transform(fn)` 对每个元素做**映射**：**元素个数不变**，遍历得到的是 `fn(elem)` 的结果。`v | transform(g)` 产生的是新值序列，不改动原容器。
2. `filter`：**可以**修改原容器元素——它产出的是对原元素的**引用**（底层范围非 const 时是 `T&`），通过视图写入会落到原容器上。
`transform`：**不能**——它产出的是 `fn(elem)` 的结果，通常是 prvalue（新算出来的值），写它是无意义的，也不会回写到原容器。
```cpp
for (int& x : v | std::views::filter(f)) x *= 2;   // 会改到 v
for (auto y  : v | std::views::transform(g)) /* y 是结果值，改它不影响 v */;
```
3. 管道是**从左到右**流过数据的：书写顺序就是处理顺序。
`v | filter(f) | transform(g)` 语义上等价于 `transform(g)(filter(f)(v))`，所以对每个元素**先过 filter，再过 transform**（filter 不满足的元素根本不会进入 transform）。
求值又是**惰性、逐个**的：外层视图取一个元素时才逐层向下拉取，不会先跑完一遍 filter 再跑一遍 transform，因此没有中间容器。
4. 直接把**数据成员指针**传给 `views::transform` 即可（成员函数指针同样可以）：
```cpp
auto prices = orders | std::views::transform(&Order::price);
auto names  = people | std::views::transform(&Person::name);
auto calc   = ticks  | std::views::transform(&Tick::mid_price);  // 成员函数
```
成员指针是"可调用物"，`transform` 会把它当 invocable 使用——比写 lambda 更短，也更能被编译器看穿。
5. ```cpp
auto aapl_prices = orders
    | std::views::filter([](const Order& o) { return o.sym == "AAPL"; })
    | std::views::transform(&Order::price);
```
即"先筛出该合约的订单，再取出价格字段"；若要取前 N 个，再接 `| std::views::take(n)`。全程惰性、无中间容器。

</details>
