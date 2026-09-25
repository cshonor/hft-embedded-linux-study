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

<details>
<summary>参考答案</summary>

1. `auto operator<=>(const T&) const = default;` 生成**三路比较本身**，从而通过"次级比较运算符改写"得到 `<`、`>`、`<=`、`>=`。
`bool operator==(const T&) const = default;` 生成 `==`，`!=` 则由编译器改写为 `!(a == b)` 得到。
补充：按 C++20 规则，如果类**没有显式声明 `operator==`**，但声明了 **defaulted 的 `<=>`**，那么 `==` 会被**隐式声明为 defaulted**（同样的访问级别与参数列表，返回类型换成 `bool`）——所以 `struct P { int x; auto operator<=>(const P&) const = default; };` 也能直接用 `==`。手写（非 default）的 `<=>` 则不会带来 `==`。
2. 按"子对象列表"的字典序，逐个比较，遇到第一个不相等的结果即返回：
   1. **直接基类子对象**，按基类在声明中的顺序；
   2. **非静态数据成员**，按成员声明顺序。
```cpp
struct Order { int sym_id; double price; int qty;
    auto operator<=>(const Order&) const = default; };
// 顺序：sym_id → price → qty
```
所以**成员声明顺序就是比较优先级**：日期想按 `year → month → day` 比较，就必须按这个顺序声明成员（否则要手写 `<=>`）。
3. 由各成员（及基类）的 `<=>` 返回类型**合成**：取其中"最弱"的那个类别，按 `strong_ordering` → `weak_ordering` → `partial_ordering` 的顺序退化。
   - 全是 `strong_ordering`（如全是 `int`）→ 结果 `strong_ordering`；
   - 混入 `weak_ordering` → 结果 `weak_ordering`；
   - 混入 `partial_ordering`（如有一个 `double` 成员）→ 结果 `partial_ordering`。
若某个成员根本不支持 `<=>`，则默认比较被定义为 **deleted**（`auto` 占位返回类型推导失败）。
4. 指 C++20 的**反转候选（reversed candidates）**：对 `a == b`，重载候选集除了 `operator==(a, b)`，还会额外合成一个**操作数顺序颠倒**的候选 `operator==(b, a)`。
于是只要"某一侧"定义了可用的 `==`，写法反过来也能用，不必为 `T == U` 和 `U == T` 各写一个重载：
```cpp
struct Value { int v; bool operator==(const Value&) const = default; };
Value x{42};
x == Value{42};   // 正常候选
// 反转候选让 Value{42} == x 这类写法也走同一个函数
```
配套的还有"次级比较运算符改写"：`!=` 由 `==` 取反得到，`<`、`>`、`<=`、`>=` 由 `<=>` 得到——所以只写 `==` 和 `<=>` 就覆盖了全部六个比较运算符。
5. 不写 `= default`，而是手写 `==` 和 `<=>`，函数体里只比较关心的成员：
```cpp
struct Person {
    std::string name;
    int age;
    bool operator==(const Person& o) const { return name == o.name; }
    auto operator<=>(const Person& o) const { return name <=> o.name; }
};
Person a{"Alice", 30}, b{"Alice", 25};
a == b;   // true（只比 name）
a <  b;   // false（name 相等）
```
要点：`==` 与 `<=>` 的**语义必须一致**（都只看 name），否则会出现"相等却互不小于"这种自相矛盾的结果，算法（sort/find/unique）会得到错误行为。

</details>
