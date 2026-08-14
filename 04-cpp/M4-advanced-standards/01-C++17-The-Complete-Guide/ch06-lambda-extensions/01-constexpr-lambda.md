# 6.1 constexpr lambda

> 第 6 章 Lambda 扩展 · 下一节：[6.2 捕获 this 值](02-捕获this值.md)

## 这节讲什么

C++17 让 lambda 可以是 `constexpr`——在编译期求值。这打开了编译期计算的新大门：用 lambda 写编译期排序、查找、校验，替代繁琐的模板元编程。

## 基本语法

```cpp
// constexpr lambda
constexpr auto square = [](int x) { return x * x; };

// 编译期调用
static_assert(square(5) == 25);  // ✅ 编译期求值

// 运行期也能用
int y = square(10);  // 运行期调用
```

## constexpr lambda 的规则

1. 如果 lambda 体满足 constexpr 函数的要求，它**自动**是 constexpr
2. 可以显式写 `constexpr` 前缀
3. 捕获的变量必须是 constexpr 可用的（不能捕获运行期变量做编译期计算）

```cpp
// 自动 constexpr
auto add = [](int a, int b) { return a + b; };
static_assert(add(2, 3) == 5);  // ✅ 自动 constexpr

// 显式 constexpr
constexpr auto mul = [](int a, int b) constexpr { return a * b; };
static_assert(mul(2, 3) == 6);
```

## 捕获的限制

```cpp
int x = 10;
auto f = [x](int v) { return v + x; };  // 捕获 x

// static_assert(f(5) == 15);  // ❌ x 不是 constexpr，不能编译期调用
int y = f(5);  // ✅ 运行期调用没问题

// 要编译期用，捕获的必须是编译期已知的值
constexpr int c = 10;
auto g = [c](int v) constexpr { return v + c; };
static_assert(g(5) == 15);  // ✅
```

## 实际用法

### 编译期表查找

```cpp
// 编译期生成查找表
constexpr auto make_table = []() {
    std::array<int, 10> t{};
    for (int i = 0; i < 10; ++i) t[i] = i * i;
    return t;
};

constexpr auto squares = make_table();
static_assert(squares[5] == 25);
static_assert(squares[9] == 81);
```

### 编译期字符串校验

```cpp
// 编译期检查字符串长度
constexpr auto check_len = [](const char* s, size_t max) constexpr {
    size_t len = 0;
    while (s[len]) ++len;
    return len <= max;
};

static_assert(check_len("hello", 10));  // ✅
// static_assert(check_len("hello world", 5));  // ❌ 编译失败
```

### 替代模板元编程

```cpp
// C++14 模板元编程：编译期阶乘
template<int N> struct Fact { static constexpr int v = N * Fact<N-1>::v; };
template<> struct Fact<0> { static constexpr int v = 1; };
static_assert(Fact<5>::v == 120);

// C++17 constexpr lambda：更直观
constexpr auto fact = [](int n) constexpr {
    int r = 1;
    for (int i = 1; i <= n; ++i) r *= i;
    return r;
};
static_assert(fact(5) == 120);  // 更简洁！
```

## 在 constexpr 函数中使用 lambda

```cpp
// constexpr 函数内定义 lambda
constexpr int sum_squares(int n) {
    auto sq = [](int x) { return x * x; };  // 自动 constexpr
    int sum = 0;
    for (int i = 1; i <= n; ++i) sum += sq(i);
    return sum;
}

static_assert(sum_squares(3) == 14);  // 1+4+9
```

## HFT 关联

```cpp
// 编译期计算固定的查找表（如校验和表）
constexpr auto crc_table = []() {
    std::array<uint32_t, 256> t{};
    for (int i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (int j = 0; j < 8; ++j)
            c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
        t[i] = c;
    }
    return t;
}();

// 编译期生成，运行时直接用，零初始化开销
uint32_t crc32(const uint8_t* data, size_t len) {
    uint32_t c = 0xFFFFFFFF;
    for (size_t i = 0; i < len; ++i)
        c = crc_table[(c ^ data[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFF;
}
```

## 小结

| 特性 | C++14 | C++17 |
|------|-------|-------|
| constexpr lambda | ❌ | ✅ |
| 自动 constexpr | — | 满足条件自动生效 |
| 捕获限制 | — | 只能捕获 constexpr 值做编译期用 |
| 替代模板元编程 | — | ✅ 更直观 |

---

← [本章导读](./README.md) · [下一节 →](02-捕获this值.md)
