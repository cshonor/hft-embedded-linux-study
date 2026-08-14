# 默认比较

## 一个 default 替代六个运算符

```cpp
struct Point {
    int x, y;
    auto operator<=>(const Point&) const = default;  // 生成 <, >, <=, >=
    bool operator==(const Point&) const = default;    // 生成 ==, !=
};

Point a{1, 2}, b{3, 4};
a < b;   // true
a == b;  // false
a != b;  // true
a >= b;  // false
```

## 生成规则

```cpp
// operator<=>() = default
// → 逐成员比较，按声明顺序
// → 先比 x，相等再比 y，以此类推
// → 返回类型由成员的比较类别决定（取最弱的）

struct Mixed {
    int i;          // strong_ordering
    double d;       // partial_ordering
    // auto operator<=>(const Mixed&) const = default;
    // 返回 partial_ordering（最弱）
};

// operator==() = default
// → 逐成员 ==
// → 独立于 <=>，需要单独声明
```

## C++20 的 == 反向推导

```cpp
struct Value {
    int v;
    bool operator==(const Value&) const = default;
};

Value x{42};
// C++20：x == 42 也合法（推导 42 → Value{42}）
// C++17：需要写 x == Value{42}
```

## 不全部 default 的情况

```cpp
struct Person {
    std::string name;
    int age;

    // 只按 name 比较，忽略 age
    bool operator==(const Person& other) const {
        return name == other.name;
    }
    auto operator<=>(const Person& other) const {
        return name <=> other.name;
    }
};

Person a{"Alice", 30}, b{"Alice", 25};
a == b;  // true（只比 name）
a < b;   // false（name 相等）
```

## HFT 应用

```cpp
struct Order {
    int sym_id;
    double price;
    int qty;

    // 自动生成比较：先 sym_id，再 price，再 qty
    auto operator<=>(const Order&) const = default;
    bool operator==(const Order&) const = default;
};

// 排序、查找、去重都有比较运算符
std::vector<Order> orders;
std::sort(orders.begin(), orders.end());
std::find(orders.begin(), orders.end(), Order{1, 100.0, 10});
```

## 自测题

1. `operator<=>() = default` 生成哪些运算符？`operator==() = default` 呢？
2. 默认比较的成员比较顺序是什么？
3. 返回类型怎么确定？（混合 strong/partial 成员）
4. C++20 的 `==` 反向推导是什么？
5. 只按部分成员比较时怎么写？
