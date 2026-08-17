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
