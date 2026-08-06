# 第 19 章 非类型模板参数扩展

**Non-Type Template Parameter (NTTP) Extensions**

## 本章讲什么

C++20 扩展了非类型模板参数（NTTP）能用的类型——支持**浮点数**和**字面量类类型**（有 constexpr 构造和成员的类）。C++17 只支持整型/指针/引用。

## 要点

### 浮点 NTTP

```cpp
// C++17：不支持浮点 NTTP
// template <double D> struct X {};  // C++17 错误

// C++20：支持
template <double D>
struct DoubleConstant {
    static constexpr double value = D;
};

DoubleConstant<3.14> x;
std::cout << x.value;   // 3.14
```

### 字面量类类型 NTTP

```cpp
// C++20：字面量类可作 NTTP（有 constexpr 构造、public 成员、无虚函数）
struct Coord {
    int x, y;
    constexpr Coord(int x, int y) : x(x), y(y) {}
    constexpr bool operator==(const Coord&) const = default;
};

template <Coord C>
struct Point {
    static constexpr Coord pos = C;
};

Point<Coord{1, 2}> p;
std::cout << p.pos.x;   // 1
```

### 固定字符串 NTTP（C++20 简化）

```cpp
// C++20：FixedString 是字面量类，直接可作 NTTP
template <FixedString S>
struct Named {
    static constexpr const char* name = S.data;
};

Named<"foo"> x;   // C++20 直接支持字符串字面量（FixedString 结构）
// 比 C++17 的变通更直接
```

（严格说 C++20 仍不直接支持 `template <"foo">`，但 FixedString 作为字面量类 NTTP 让写法几乎等价。）

### C++20 NTTP 的限制

NTTP 类型必须是**结构化类型**：
- 整型、枚举、指针、引用（C++17 已支持）
- 浮点（C++20 新增）
- 字面量类（C++20 新增）：有 constexpr 构造、public 非静态成员、无虚函数、所有基类/成员都是结构化类型

### 用途

```cpp
// 1. 浮点配置参数
template <double ALPHA>
struct EMA {
    double compute(double x) {
        return ALPHA * x + (1 - ALPHA) * prev;
    }
};
EMA<0.1> ema;   // 编译期确定 EMA 系数

// 2. 坐标作模板参数
template <Coord ORIGIN>
struct Grid {
    static constexpr Coord origin = ORIGIN;
};

// 3. 字符串名
template <FixedString NAME>
struct Strategy {
    static constexpr const char* name = NAME.data;
};
```

## HFT 关联

- **浮点策略参数**：`EMA<0.1>` 的系数编译期确定，编译器可做常量折叠优化（`0.1 * x` 可能编译为一条 `mulsd` 带立即数）。
- **FixedString 策略名**：`Strategy<"Alpha">` 类型带名字，日志/调试零开销（C++20 FixedString NTTP 比 C++17 更自然）。
- **坐标作模板参数**：网格策略的网格原点 `Grid<Coord{0,0}>` 编译期确定。
- **编译期常量优化**：NTTP 值编译期已知，编译器可激进内联和常量折叠。
- **替代宏定义**：`#define ALPHA 0.1` 用 `EMA<0.1>` 替代，类型安全、作用域可控。

## 自测题

1. C++20 NTTP 相比 C++17 新增了哪些类型？
2. 浮点 NTTP 有什么用？HFT 策略参数怎么用？
3. 字面量类作 NTTP 的条件是什么？
4. C++20 的 FixedString NTTP 比 C++17 的变通好在哪里？
5. NTTP 编译期已知值如何帮助编译器优化？
