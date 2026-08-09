# 第 7 章 类

类是 C++ 最基本的特性，核心思想是**数据抽象（Data Abstraction）**和**封装（Encapsulation）**。本章引导读者从零设计类，控制对象的初始化与权限。

## 小节

- [定义抽象数据类型](./7.1-定义抽象数据类型.md)
- [访问控制与封装](./7.2-访问控制与封装.md)
- [友元（Friend）](./7.3-友元（Friend）.md)
- [类的其他特性](./7.4-类的其他特性.md)
- [构造函数（Constructors）](./7.5-构造函数（Constructors）.md)
- [类的静态成员（Static Members）](./7.6-类的静态成员（StaticMembers）.md)


## 章节摘要

类的核心概念：数据抽象与封装。包括定义抽象数据类型、`this` 指针、访问控制（`public`/`private`/`protected`）、友元、构造函数、类的静态成员。

### 和 C 的区别

| C struct | C++ class |
|----------|-----------|
| 只有数据 | 数据 + 函数（成员函数） |
| 无访问控制 | `public`/`private`/`protected` |
| 无构造/析构 | 自动调用构造/析构 |
| 无 `this` | 隐式 `this` 指针 |
| 无静态成员 | `static` 成员 |

## 章节自测

### Q1: this 指针

```cpp
class Counter {
    int count = 0;
public:
    Counter& increment() {
        ++count;
        return *this;
    }
    int get() const { return count; }
};
int main() {
    Counter c;
    c.increment().increment().increment();
    std::cout << c.get();
}
```

> 输出是什么？`*this` 是什么意思？`const` 成员函数的 `this` 是什么类型？

<details>
<summary>答案与复习指引</summary>

**输出：** `3`

- `*this` 是当前对象的引用——`increment` 返回 `*this` 允许链式调用 `c.increment().increment().increment()`
- `const` 成员函数的 `this` 是 `const Counter*`——不能修改成员（除非成员是 `mutable`）
- 非 `const` 成员函数的 `this` 是 `Counter*`

**复习：** → [定义抽象数据类型](./7.1-定义抽象数据类型.md)
</details>

### Q2: 访问控制

```cpp
class Account {
    std::string owner;      // 默认 private
    double balance = 0;
public:
    void deposit(double amt) { balance += amt; }
    double get_balance() const { return balance; }
};
struct PodAccount {
    std::string owner;      // 默认 public
    double balance = 0;
};
```

> `class` 和 `struct` 的唯一区别是什么？

<details>
<summary>答案与复习指引</summary>

**唯一区别：** 默认访问权限——`class` 默认 `private`，`struct` 默认 `public`。其余完全相同（都可以有成员函数、构造/析构、继承等）。

**实践：** 纯数据聚合用 `struct`（公开数据），有不变式/封装需求用 `class`（私有数据 + 公开接口）。

**复习：** → [访问控制与封装](./7.2-访问控制与封装.md)
</details>

### Q3: 构造函数

```cpp
class Point {
    int x, y;
public:
    Point() : x(0), y(0) {}           // A: 默认构造
    Point(int a, int b) : x(a), y(b) {}  // B: 带参构造
};
int main() {
    Point p1;           // 调用 A
    Point p2(3, 4);     // 调用 B
    Point p3();         // 调用...？
}
```

> `p3` 是什么？

<details>
<summary>答案与复习指引</summary>

**`p3` 是函数声明！** 声明了一个名为 `p3`、无参返回 `Point` 的函数。这就是"最烦人解析"（Most Vexing Parse）。

**正确写法：** `Point p3{};`（C++11 花括号初始化）或 `Point p3;`（不写括号）。

**复习：** → [构造函数（Constructors）](./7.5-构造函数（Constructors）.md)
</details>

### Q4: 友元

```cpp
class Wallet {
    int money = 100;
    friend void audit(const Wallet &w);  // 友元函数
};
void audit(const Wallet &w) {
    std::cout << w.money;  // 合法：友元可访问私有成员
}
```

> 友元函数和成员函数有什么区别？友元破坏封装吗？

<details>
<summary>答案与复习指引</summary>

**区别：**
- 友元函数不是类的成员，但可以访问私有成员
- 友元函数没有 `this` 指针
- 友元声明在类内，但定义在类外

**是否破坏封装：** 友元是**有控制的**开放——由类自己决定谁是友元，不是随便谁都能访问。比 `public` 所有成员更安全。典型用途：运算符重载（`operator<<` 需要访问私有成员但不是成员函数）。

**复习：** → [友元（Friend）](./7.3-友元（Friend）.md)
</details>

### Q5: 静态成员

```cpp
class Bank {
    static int total_accounts;
public:
    Bank() { ++total_accounts; }
    static int get_total() { return total_accounts; }
};
int Bank::total_accounts = 0;  // 类外定义
int main() {
    Bank b1, b2, b3;
    std::cout << Bank::get_total();
}
```

> 输出是什么？静态成员和全局变量有什么区别？

<details>
<summary>答案与复习指引</summary>

**输出：** `3`

**和全局变量的区别：**
1. 静态成员受类作用域约束（`Bank::total_accounts`），不污染全局命名空间
2. 受访问控制约束（可以是 `private`）
3. 可以是 `private` 的，只有类成员/友元能访问

**注意：** 静态成员变量必须在类外定义（分配存储），C++17 起可用 `inline static` 在类内定义。

**复习：** → [类的静态成员（Static Members）](./7.6-类的静态成员（StaticMembers）.md)
</details>
