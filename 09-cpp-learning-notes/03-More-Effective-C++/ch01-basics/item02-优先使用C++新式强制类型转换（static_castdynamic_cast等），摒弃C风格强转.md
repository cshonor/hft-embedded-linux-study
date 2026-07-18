# 条款 2：优先使用 C++ 新式强制类型转换（static_cast/dynamic_cast 等），摒弃 C 风格强转

## 本节讲什么

C 风格强转 `(type) expression` 在紧要关头有时不得不用，但**过于粗鲁、难以检索**。C++ 引入四个语义更精确的新式转换符，让「转换意图」在代码里一眼可见，也便于编译器做更严格的检查。本条款对比 C 风格强转的缺陷，并说明四种 `*_cast` 的适用边界。

## C 风格强转的两处缺陷

### 1. 过于粗鲁，缺乏精确性

C 风格转换允许在几乎任意类型之间互转，**不区分转换目的**——究竟是去掉 `const`，还是彻底改变了对象类型，语法上完全一样。

### 2. 难以识别

`(type) expr` 只是一对圆括号加一个标识符，人工阅读和 `grep` 搜索都很难在大量代码里准确筛出「哪里做了类型转换」。

```cpp
double d = 3.14;
int i = (int)d;           // C 风格：意图不明
// int j = static_cast<int>(d);  // 新式：一眼看出是数值转换
```

## 四种 C++ 新式转换符

| 转换符 | 主要用途 | 关键限制 |
|--------|----------|----------|
| **`static_cast`** | 普通、编译期类型转换（如 `int`→`double`） | 不能 `struct`→`int`、不能 `double`→指针、**不能去 `const`** |
| **`const_cast`** | **仅**去掉或添加 `const` / `volatile` | 不能用于其他类型转换 |
| **`dynamic_cast`** | 沿继承体系**安全向下转型** | 需多态（虚函数）；不能去 `const`；唯一可运行时判定成败 |
| **`reinterpret_cast`** | 底层、实现定义的重解释（如函数指针互转） | 可移植性极差，**除非别无选择否则避免** |

### static_cast

功能上基本替代 C 风格强转中的「普通转换」，但范围更受限，语法：`static_cast<type>(expression)`。

```cpp
double d = 3.14;
int i = static_cast<int>(d);

// static_cast 不能去 const：
// const int ci = 0;
// int *p = static_cast<int*>(&ci);  // 错误
```

### const_cast

**专门**用于转换表达式的 `const` 或 `volatile` 属性；若试图用它做别的转换，编译器会拒绝。

```cpp
void f(int *p);
void g(const int *cp) {
    f(const_cast<int *>(cp));  // 去掉 const，调用方需保证不修改只读对象
}
```

### dynamic_cast

用于把指向**基类**的指针/引用安全转为指向**派生类**的指针/引用；是唯一能**在运行时知道转换是否成功**的操作符：

- **指针**：失败返回 `nullptr`
- **引用**：失败抛出 `std::bad_cast`

要求类型具备多态（通常基类有虚函数）；不能去 `const`。

```cpp
class Base { public: virtual ~Base() = default; };
class Derived : public Base {};

Base *bp = new Derived;
Derived *dp = dynamic_cast<Derived *>(bp);  // 成功
if (!dp) { /* 向下转型失败 */ }

Base &br = *bp;
try {
    Derived &dr = dynamic_cast<Derived &>(br);
} catch (const std::bad_cast &) {
    // 引用转型失败
}
```

### reinterpret_cast

底层重解释，结果几乎总是**实现定义**的，代码极难移植。常见场景是不同函数指针类型之间的转换。书中形容为「一把非常锋利的刀」——除非别无选择，否则避免。

```cpp
// 典型但危险：函数指针类型互转（可移植性无保证）
// auto fp = reinterpret_cast<void(*)()>(some_func_ptr);
```

## 为什么新式转换「丑陋」反而是好事？

`static_cast<int>(x)` 比 `(int)x` 更长、更丑，但这正是优点：

1. **可辨认**：类型转换在代码里非常显眼，便于审查与搜索。
2. **编译器友好**：语法结构清晰，错误检测更精确。
3. **心理警告**：键入繁琐提醒程序员——类型转换是在**破坏类型系统**，应尽量少用。

## 老式编译器：用宏过渡

若编译器尚不支持新式转换符，可用宏模拟（不如真操作符安全，但便于日后升级）：

```cpp
#define static_cast(TYPE, EXPR)   ((TYPE)(EXPR))
#define const_cast(TYPE, EXPR)    ((TYPE)(EXPR))
#define reinterpret_cast(TYPE, EXPR) ((TYPE)(EXPR))
// dynamic_cast 无法用宏完美模拟——缺少运行时判定转换失败的机制
```

## 示例

```cpp
#include <iostream>

class Widget {
public:
    virtual ~Widget() = default;
};
class SpecialWidget : public Widget {};

int main() {
    // 数值转换 → static_cast
    double pi = 3.14;
    int n = static_cast<int>(pi);

    // 去 const → const_cast（需保证语义合法）
    const int ci = 42;
    int &ri = const_cast<int &>(ci);

    // 多态向下转型 → dynamic_cast
    Widget *w = new SpecialWidget();
    SpecialWidget *sw = dynamic_cast<SpecialWidget *>(w);
    if (sw) std::cout << "downcast ok\n";
    delete w;

    // 避免：(int)pi、(SpecialWidget*)w 等 C 风格强转
    return 0;
}
```

## 小结

- C 风格强转：意图模糊、难以检索，应优先改用四种 `*_cast`。
- **`static_cast`**：普通编译期转换；**`const_cast`**：只动 const/volatile；**`dynamic_cast`**：安全向下转型；**`reinterpret_cast`**：底层重解释，慎用。
- 新式转换故意「惹眼」，是为了让危险操作难以被忽略；转换越少越好。
