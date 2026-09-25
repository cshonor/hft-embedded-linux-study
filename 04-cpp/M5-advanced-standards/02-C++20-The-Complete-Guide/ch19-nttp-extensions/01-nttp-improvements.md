# NTTP 扩展

## 非类型模板参数

```cpp
// C++17：NTTP 只能是整数/枚举/指针/引用
template <int N>
struct Array { int data[N]; };

template <auto N>  // C++17：auto 推导类型
struct Value { static constexpr auto val = N; };

Value<42> v1;        // N = int, 42
Value<'a'> v2;       // N = char, 'a'
Value<3.14> v3;      // ❌ C++17 不支持 double
```

## C++20：类类型 NTTP

```cpp
// C++20：NTTP 可以是字面量类类型
struct FixedString {
    char data[16];
    size_t len;

    constexpr FixedString(const char* s) : len(0) {
        for (; s[len] && len < 15; ++len) data[len] = s[len];
        data[len] = '\0';
    }

    constexpr bool operator==(const FixedString& other) const {
        // 比较...
    }
};

// 用字符串做模板参数！
template <FixedString Name>
struct NamedValue {
    static constexpr FixedString name = Name;
    int value = 0;
};

NamedValue<"AAPL"> a;  // 模板参数是字符串
NamedValue<"GOOG"> g;
```

## 固定字符串模板参数

```cpp
// C++20 最实用的 NTTP：编译期字符串
template <FixedString Tag>
class TaggedCounter {
    int count = 0;
public:
    void inc() { ++count; }
    void print() { std::cout << Tag.data << ": " << count << '\n'; }
};

TaggedCounter<"orders"> order_counter;
TaggedCounter<"fills"> fill_counter;

order_counter.inc();
order_counter.print();  // "orders: 1"
```

## 浮点 NTTP

```cpp
// C++20：浮点数可以作为 NTTP
template <double Pi>
struct CircleArea {
    static constexpr double area(double r) { return Pi * r * r; }
};

CircleArea<3.14159> c;
c.area(1.0);  // 3.14159
```

## 实际应用

```cpp
// 编译期策略选择：用字符串选择策略
template <FixedString StrategyName>
auto create_strategy() {
    if constexpr (StrategyName == FixedString{"momentum"}) {
        return MomentumStrategy{};
    } else if constexpr (StrategyName == FixedString{"mean_reversion"}) {
        return MeanReversionStrategy{};
    }
}

auto strat = create_strategy<"momentum">();
```

## 自测题

1. C++17 的 NTTP 能用什么类型？有什么限制？
2. C++20 允许什么类型的 NTTP？
3. 类类型 NTTP 需要满足什么条件？
4. 用 `FixedString` 做模板参数有什么用？
5. C++20 浮点 NTTP 的写法？

<details>
<summary>参考答案</summary>

1. C++17（沿用 C++11）的 NTTP 只能是这些**结构化类型**：整型（含 `bool`、`char` 等）、枚举类型、指针/引用（指向具有链接的静态对象或函数）、成员指针、`std::nullptr_t`。
限制很明显：**不能是浮点**、**不能是类类型**（哪怕是字面类）、**不能是字符串字面量**（只能退而求其次用 `extern const char[]` 的引用或指针）；而且模板实参的相等性按"值/同一实体"比较，写法上也非常受限。
2. C++20 把 NTTP 放宽为"**任何结构化类型（structural type）**"，新增了：
   - **浮点类型**（`float`/`double`/`long double`）；
   - **字面类类型**（literal class type，只要满足结构化条件）及其数组，例如 `std::array<char, N>`；
   - 由此可以做出 `FixedString` 这类"编译期字符串"NTTP。
仍然不行的是**字符串字面量本身**（`T<"abc">` 只有在参数是能捕获它的类类型时才成立）、以及非结构化的类（有 private 成员、`mutable` 成员，或含非结构化成员）。
3. 类类型要能作 NTTP，必须是**结构化类型（structural type）**，条件有三条（递归判定）：
   1. 它是**字面类型**（literal type：析构是 constexpr/平凡的、可直接析构）；
   2. 它的**所有基类和非静态数据成员都是 public 且非 `mutable`**；
   3. 这些基类与成员的类型本身也都是**结构化类型**。
```cpp
template <std::size_t N>
struct FixedString {
    char data[N]{};                       // public、非 mutable、成员是结构化类型
    constexpr FixedString(const char (&s)[N]) { /* 拷贝 */ }
};
```
满足后，模板实参的**相等性按成员逐一比较**——这正是 `FixedString` 能做 `==` 与 `if constexpr` 分发的基础。
4. 它让"编译期字符串"成为模板参数，用途包括：
   1. **给类型打标签**：`TaggedCounter<"orders">` 与 `TaggedCounter<"fills">` 是**不同类型**，各自有独立的静态状态，零运行期开销；
   2. **编译期策略/实现选择**：
```cpp
template <FixedString Name>
auto create_strategy() {
    if constexpr (Name == FixedString{"momentum"})      return MomentumStrategy{};
    else if constexpr (Name == FixedString{"mean_reversion"}) return MeanReversionStrategy{};
}
auto s = create_strategy<"momentum">();
```
   3. 日志/指标的名字内联进类型，省掉运行期字符串比较与存储；
   4. 让不同 tag 的实例在编译期就被区分，便于静态断言与代码生成。
5. ```cpp
template <double Pi>
struct CircleArea {
    static constexpr double area(double r) { return Pi * r * r; }
};

CircleArea<3.14159> c;      // 浮点字面量直接作 NTTP
c.area(1.0);                // 3.14159
```
注意事项：浮点 NTTP 的相等性按**值**比较——两个字面量数值相同就是同一个类型（`CircleArea<3.14>` 与 `CircleArea<3.140>` 是同一类型）；但 **NaN 与自身不相等**，所以 `CircleArea<NaN>` 每次都会得到不同的类型，实际中应避免。

</details>
