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
