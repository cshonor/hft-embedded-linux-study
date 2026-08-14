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
