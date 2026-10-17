# <=> 三路比较运算符

## 基本用法

```cpp
#include <compare>

auto result = 1 <=> 2;
// result < 0：1 < 2
// result == 0：相等
// result > 0：大于

// 配合 if
if (auto c = a <=> b; c < 0) {
    // a < b
} else if (c > 0) {
    // a > b
} else {
    // a == b
}
```

## 返回类型：比较类别

```cpp
// 整数：strong_ordering
auto r1 = 1 <=> 2;  // std::strong_ordering::less

// 浮点：partial_ordering（NaN 不可比）
auto r2 = 1.0 <=> 2.0;  // std::partial_ordering::less

// 自定义类型：取决于 <=> 的返回类型
struct Point {
    int x, y;
    auto operator<=>(const Point&) const = default;  // strong_ordering
};
```

## 三个比较类别

```cpp
// 1. strong_ordering：强序
//    1 != 2 != 3，每个值唯一
//    less, equal, greater
std::strong_ordering::less
std::strong_ordering::equal
std::strong_ordering::greater

// 2. weak_ordering：弱序
//    大小写不敏感：A == a，但 A 不等同 a
//    less, equivalent, greater
std::weak_ordering::less
std::weak_ordering::equivalent
std::weak_ordering::greater

// 3. partial_ordering：偏序
//    浮点：NaN 不可比
//    less, equivalent, greater, unordered
std::partial_ordering::less
std::partial_ordering::equivalent
std::partial_ordering::greater
std::partial_ordering::unordered
```

## 类别层次

```
strong_ordering → weak_ordering → partial_ordering
（强序蕴含弱序蕴含偏序）
```

`strong_ordering` 可以隐式转为 `weak_ordering`，后者可以转为 `partial_ordering`。

## 自定义 <=>

```cpp
struct Version {
    int major, minor, patch;

    // 自定义比较逻辑
    auto operator<=>(const Version& other) const {
        if (auto c = major <=> other.major; c != 0) return c;
        if (auto c = minor <=> other.minor; c != 0) return c;
        return patch <=> other.patch;
    }
    bool operator==(const Version&) const = default;
};

Version v1{1, 2, 3}, v2{1, 3, 0};
v1 < v2;   // true（1.2.3 < 1.3.0）
v1 == v2;  // false
```

## 自测题

1. `<=>` 返回什么类型？和 `<` 返回 bool 有什么区别？
2. 三个比较类别分别是什么？各自适用什么场景？
3. `strong_ordering` 和 `weak_ordering` 的区别？
4. 浮点用哪个比较类别？为什么？
5. 自定义类型的 `<=>` 怎么写？

<details>
<summary>参考答案</summary>

1. 它返回**比较类别类型**（comparison category），而不是 `bool`：`std::strong_ordering`、`std::weak_ordering` 或 `std::partial_ordering`（具体哪一种由操作数决定，如 `1 <=> 2` 得到 `std::strong_ordering`，`1.0 <=> 2.0` 得到 `std::partial_ordering`）。
区别在于信息量：`<` 只能回答"是/否"，而 `<=>` 一次比较就能给出"小于 / 等价 / 大于"（偏序还有"不可比"）三种结果。
实际收益：写一个 `<=>` 就同时支撑 `<`、`>`、`<=`、`>=` 四个运算符（由编译器改写），不必手写六个比较运算符。
2. 三个类别：
   - **`std::strong_ordering`**（强序）：全序，且"等价"就意味着**完全相等、可互相替换**。值：`less` / `equal`（也写作 `equivalent`）/ `greater`。适用：整数、指针、按字典序且大小写敏感的字符串。
   - **`std::weak_ordering`**（弱序）：全序，但"等价"只表示序上分不出大小，两个值**未必可互换**。值：`less` / `equivalent` / `greater`。适用：大小写不敏感的字符串（`"A"` 与 `"a"` 等价但并不等同）。
   - **`std::partial_ordering`**（偏序）：存在**不可比**的元素。值：`less` / `equivalent` / `greater` / `unordered`。适用：浮点（NaN 不可比）。
三者的强弱关系是 `strong_ordering` → `weak_ordering` → `partial_ordering`（前者可隐式转换成后者）。
3. 两者都是全序，差别在"等价"的含义：
   - `strong_ordering` 的 `equal` 表示两个值**不可区分**——任何可观察行为都一样，可以互相替换（`a` 与 `b` 等价则 `f(a) == f(b)`）。
   - `weak_ordering` 的 `equivalent` 只表示"在这个序下既不小于也不大于"，二者**仍可能是不同的值**（如大小写不敏感比较下 `"A"` 与 `"a"`）。
因此 strong_ordering 更强：如果你的类型用 strong_ordering，标准库（排序、查找、去重）可以做更强的假设；而 weak_ordering 适合"按某个投影排序、但其他方面不同"的场景。
4. 浮点用 **`std::partial_ordering`**。
原因：IEEE-754 的 **NaN 与任何值（包括它自己）都不可比**，全序的前提不成立，必须有一个表示"不可比"的结果，这就是 `std::partial_ordering::unordered`。
```cpp
1.0 <=> 2.0;                 // partial_ordering::less
std::numeric_limits<double>::quiet_NaN() <=> 1.0;  // partial_ordering::unordered
```
因此内置 `double <=> double` 的结果就是 `std::partial_ordering`；把它放进 `std::map`/`std::sort` 时也要意识到 NaN 会破坏严格弱序。
5. 返回比较类别，并逐成员比较（短路返回第一个非零结果）：
```cpp
struct Version {
    int major, minor, patch;
    auto operator<=>(const Version& o) const {
        if (auto c = major <=> o.major; c != 0) return c;
        if (auto c = minor <=> o.minor; c != 0) return c;
        return patch <=> o.patch;
    }
    bool operator==(const Version&) const = default;   // == 需单独提供
};
```
要点：返回类型用 `auto` 让编译器推导；**`<=>` 不会自动生成 `==`**（除非是 `= default` 的 `<=>`，见下一节），所以通常要同时写 `operator==`；比较类别要与语义匹配（含 `double` 成员时结果会退化为 `partial_ordering`）。

</details>
