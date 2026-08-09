# 条款 5：警惕编译器隐式类型转换函数带来的意外调用

## 本节讲什么

编译器可在内建类型间隐式转换（如 `char`→`int`）。自定义类时，可通过**单参数构造函数**和**隐式类型转换运算符**让编译器做用户定义的类型转换。二者在「不需要转换时」也常被悄悄调用，导致难以察觉的错误。本条款说明两类机制的弊端，以及 `explicit`、显式成员函数、代理类等对策。

## 两种隐式转换入口

| 机制 | 典型形式 | 效果 |
|------|----------|------|
| **单参数构造函数** | `Array(int size);` | 允许 `Array a = 10;` 等隐式从 `int` 构造 |
| **隐式转换运算符** | `operator double() const;` | 允许在需要 `double` 处隐式使用类对象 |

**原则**：除非确实迫切需要，**不要定义类型转换函数**；隐式转换的弊端往往大于便利。

## 1. 隐式类型转换运算符

### 弊端

为 `Rational` 定义 `operator double()` 后，若忘记重载 `operator<<`，写 `cout << r` 时编译器**不报错**——会悄悄把有理数转成 `double` 打印，完全违背预期。

```cpp
class Rational {
    int num_, den_;
public:
    Rational(int n, int d) : num_(n), den_(d) {}

    // 隐式转换运算符 — 危险
    operator double() const {
        return static_cast<double>(num_) / den_;
    }
};

// std::ostream &operator<<(std::ostream &os, const Rational &r);

Rational r(1, 2);
// std::cout << r;  // 不报错！悄悄调用 operator double()，打印 0.5
```

### 对策：显式成员函数

用普通成员函数替代隐式转换语法，只在程序员**明确调用**时才转换。`std::string` 不提供向 `char*` 的隐式转换，而是要求显式调用 `c_str()`。

```cpp
class Rational {
    int num_, den_;
public:
    Rational(int n, int d) : num_(n), den_(d) {}

    double asDouble() const {  // 显式，不会参与隐式匹配
        return static_cast<double>(num_) / den_;
    }
};

// std::cout << r.asDouble();  // 意图清晰
```

## 2. 单参数构造函数

单参数构造造成的隐式转换通常**比转换运算符更难消除**，问题也更严重。

### 弊端

模板类 `Array` 有 `Array(int size)` 时，若比较写错成 `a == b[i]`（本意 `a[i] == b[i]`），编译器不报错：会把 `int` 类型的 `b[i]` **隐式构造成临时 `Array`** 再与 `a` 比较——逻辑荒谬，还产生大量临时对象构造/析构开销。

```cpp
class Array {
    int *data_;
    int size_;
public:
    Array(int size) : data_(new int[size]), size_(size) {}
    ~Array() { delete[] data_; }
    bool operator==(const Array &other) const { /* ... */ return true; }
};

void bug(Array &a, Array &b, int i) {
    if (a == b[i]) {  // 本意 a[i] == b[i]；实际：Array 临时对象 + 荒谬比较
        // ...
    }
}
```

### 对策一（最佳）：`explicit`

用 **`explicit`** 声明单参数构造函数，拒绝为隐式转换而调用，仍允许显式转换（如 `static_cast`、`Array(10)`）。

```cpp
class Array {
public:
    explicit Array(int size) : data_(new int[size]), size_(size) {}
    // ...
};

// Array a = 10;           // 错误
Array a(10);                // OK
Array b = static_cast<Array>(10);  // OK：显式
// a == b[i];              // 错误：无法隐式 int → Array
```

### 对策二（老编译器）：代理类（Proxy Class）

若编译器不支持 `explicit`，可嵌套 **代理类** `ArraySize`，让构造函数只接受 `ArraySize` 而非 `int`。C++ 规定：**不能链式隐式调用超过一次**用户自定义转换，因此 `int` 无法隐式 → `ArraySize` → `Array`（详见 [条款 30](../ch06-smart-pointers/item30-代理类（ProxyClass）设计模式，解决运算符重载、容器下标等语法痛点.md)）。

```cpp
class Array {
public:
    class ArraySize {
        int size_;
        explicit ArraySize(int s) : size_(s) {}
        friend class Array;
    };

    Array(ArraySize as) : size_(as.size_) { /* ... */ }

    static ArraySize createSize(int s) { return ArraySize(s); }
};

// Array a(10);                    // 错误：int 不能隐式变 ArraySize
Array a(Array::createSize(10));     // OK：显式
```

## 对比小结

| 问题来源 | 典型坑 | 首选对策 |
|----------|--------|----------|
| 转换运算符 | `cout << obj` 悄悄转成别的类型 | 用 `asDouble()` / `c_str()` 等显式 API |
| 单参数构造 | 漏写 `[i]` 导致隐式构造临时对象 | **`explicit`** 单参数构造 |
| 无 `explicit` 的老环境 | 同上 | 代理类阻断二重隐式转换 |

## 示例

```cpp
#include <iostream>
#include <string>

class SafeRational {
    int num_, den_;
public:
    SafeRational(int n, int d) : num_(n), den_(d) {}

    double asDouble() const {
        return static_cast<double>(num_) / den_;
    }
    // 不提供 operator double()
};

class SafeArray {
    int size_;
public:
    explicit SafeArray(int size) : size_(size) {}

    bool operator==(const SafeArray &) const { return true; }
};

int main() {
    SafeRational r(1, 3);
    std::cout << r.asDouble() << '\n';

    SafeArray a(5), b(5);
    // if (a == 5) {}  // 错误：不能隐式 int → SafeArray
    if (a == b) {}     // OK

    std::string s = "hi";
    // char *p = s;           // 错误
    const char *p = s.c_str(); // OK：显式
}
```

## 小结

- 单参数构造与隐式转换运算符都会在「未预期处」被编译器调用；后者还常静默通过编译。
- **转换运算符**：优先显式成员函数（`asDouble`、`c_str`），避免 `operator T()`。
- **单参数构造**：默认加 **`explicit`**；老编译器用代理类阻断链式隐式转换。
- 除非你确实需要隐式转换，否则不要定义类型转换函数。
