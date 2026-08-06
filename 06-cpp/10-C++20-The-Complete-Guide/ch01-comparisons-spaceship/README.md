# 第 1 章 比较与 <=> 运算符

**Comparisons and Operator <=>**

## 本章讲什么

C++20 引入**三路比较运算符 `<=>`（spaceship operator）**和**默认比较**——一个 `auto operator<=>() = default` 自动生成全部六种比较运算符，替代手写一堆 `==`/`<`/`<=` 等。

## 要点

### `<=>` 三路比较

```cpp
auto result = a <=> b;
// result < 0：a < b
// result == 0：a == b
// result > 0：a > b
```

`<=>` 返回的不是 bool，而是**比较类别**类型：

| 类别 | 类型 | 语义 |
|------|------|------|
| strong_ordering | `std::strong_ordering` | 强序（int：1 != 2 != 3） |
| weak_ordering | `std::weak_ordering` | 弱序（大小写不敏感：A == a 但 A 不等同 a） |
| partial_ordering | `std::partial_ordering` | 偏序（浮点：NaN 不可比） |

### 默认比较

```cpp
struct Point {
    int x, y;
    auto operator<=>(const Point&) const = default;   // 生成全部六种比较
    bool operator==(const Point&) const = default;     // C++20 生成 ==
};
// 现在 Point 有 ==, !=, <, >, <=, >=
```

`<=>` default 自动生成 `<`/`>`/`<=`/`>=`，`==` default 生成 `==`/`!=`。**一个 default 声明替代手写六个运算符**。

### 自定义 `<=>`

```cpp
struct Version {
    int major, minor, patch;
    auto operator<=>(const Version& other) const {
        if (auto c = major <=> other.major; c != 0) return c;
        if (auto c = minor <=> other.minor; c != 0) return c;
        return patch <=> other.patch;
    }
    bool operator==(const Version&) const = default;
};
```

逐字段比较，返回第一个非等的结果。`<=>` 的返回类型决定比较类别（这里 `strong_ordering`）。

### 合成的比较（reverse）

C++20 的 `<=>` 自动合成**反向比较**：`b < a` 会用 `a <=> b > 0` 合成。这让"左操作数是 int、右操作数是 BigInt"的比较自动工作（只要 BigInt 有 `<=>`）。

```cpp
struct BigInt {
    int val;
    auto operator<=>(const BigInt&) const = default;
};
BigInt b{42};
b < 100;    // OK：合成
100 < b;    // OK：C++20 自动反向合成（100 <=> b > 0）
```

C++17 之前要写两个重载（`operator<(BigInt, int)` 和 `operator<(int, BigInt)`）。

### `==` 的传播

C++20 的 `==` 也支持反向合成：
```cpp
b == 42;    // OK
42 == b;    // C++20 自动合成（用 b == 42）
```

## HFT 关联

- **行情/订单结构默认比较**：`struct Order { auto operator<=>(...) = default; }` 一行生成全部比较，替代手写六函数。
- **版本号比较**：策略版本 `Version` 用自定义 `<=>` 逐字段比较，语义清晰。
- **`==` 与 `<=>` 分离**：C++20 把相等（`==`）和排序（`<=>`）分离——浮点 NaN 场景 `==` 可能 false 但 `<=>` 是 partial_ordering，语义更精确。
- **减少样板代码**：HFT 数据结构多（Tick/Trade/Order/Quote），`<=>` default 大幅减少比较运算符手写。
- **性能无损失**：默认生成的比较和手写的等价，逐字段短路。

## 自测题

1. `<=>` 返回什么？三种比较类别分别是什么？
2. `auto operator<=>(...) = default` 生成哪些运算符？还需要单独声明 `==` 吗？
3. C++20 的反向比较合成是什么？解决了什么问题？
4. `==` 和 `<=>` 为什么在 C++20 分离？
5. HFT 数据结构如何用 `<=>` default 减少样板代码？
