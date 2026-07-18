#!/usr/bin/env python3
"""为 C++ 学习笔记各小节追加 ## 示例 代码块。"""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BOOKS = [
    ROOT / "01-C++Primer",
    ROOT / "02-Effective-C++",
    ROOT / "03-More-Effective-C++",
]

PRIMER_EXAMPLES: dict[str, str] = {
    "1.1": """```cpp
#include <iostream>
int main() {
    return 0;  // 返回 0 表示程序正常结束
}
```""",
    "1.2": """```cpp
#include <iostream>
int main() {
    std::cout << "Enter two numbers: ";
    int v1 = 0, v2 = 0;
    std::cin >> v1 >> v2;
    std::cout << v1 << " + " << v2 << " = " << v1 + v2 << std::endl;
    return 0;
}
```""",
    "1.3": """```cpp
// 单行注释：说明下面代码的作用
#include <iostream>
/*
 * 多行注释
 * 常用于临时屏蔽大段代码
 */
int main() {
    std::cout << "Hello" << std::endl;
    return 0;
}
```""",
    "1.4": """```cpp
#include <iostream>
int main() {
    int sum = 0, val = 1;
    while (val <= 10) {
        sum += val;
        ++val;
    }
    std::cout << "Sum 1..10 = " << sum << std::endl;
    return 0;
}
```""",
    "1.5": """```cpp
#include <iostream>
#include <string>
struct Sales_data {
    std::string bookNo;
    unsigned units_sold = 0;
    double revenue = 0.0;
};
int main() {
    Sales_data item{"978-0-201-78345-0", 25, 22.5};
    std::cout << item.bookNo << ": sold " << item.units_sold << std::endl;
    return 0;
}
```""",
    "1.6": """```cpp
#include <iostream>
#define DEBUG_MODE 1
#if DEBUG_MODE
    #define LOG(x) std::cerr << x << std::endl
#else
    #define LOG(x)
#endif
int main() {
    LOG("compiled with debug");
    return 0;
}
```""",
    "1.7": """```cpp
// 第 1 章综合：读入数据并输出
#include <iostream>
int main() {
    std::cout << "Enter numbers (Ctrl+Z then Enter to end):\\n";
    int sum = 0, value = 0;
    while (std::cin >> value)
        sum += value;
    std::cout << "Sum = " << sum << std::endl;
    return 0;
}
```""",
    "2.1": """```cpp
#include <iostream>
int main() {
    int i = 42;
    double pi = 3.14;
    bool flag = true;
    char ch = 'A';
    std::cout << i << " " << pi << " " << flag << " " << ch << std::endl;
    return 0;
}
```""",
    "2.2": """```cpp
#include <iostream>
int main() {
    int ival = 1024;
    int &refVal = ival;  // refVal 是 ival 的别名
    refVal = 2048;
    std::cout << ival << std::endl;  // 2048
    return 0;
}
```""",
    "2.3": """```cpp
#include <iostream>
int main() {
    int ival = 42;
    int *p = &ival;       // p 指向 ival
    std::cout << *p << std::endl;  // 解引用
  *p = 0;
    std::cout << ival << std::endl;  // 0
    return 0;
}
```""",
    "2.4": """```cpp
#include <iostream>
int main() {
    const int ci = 42;
    const int *p = &ci;      // 指向常量的指针
    int const *q = &ci;      // 同上
    // *p = 0;               // 错误：不能通过 p 修改 ci
    int j = 0;
    int *const r = &j;       // 常量指针，r 不能改指向
    *r = 100;
    return 0;
}
```""",
    "2.5": """```cpp
#include <iostream>
int main() {
    int ival = 0;
    {   // 作用域开始
        int ival = 42;  // 内层 ival 遮蔽外层
        std::cout << ival << std::endl;
    }
    std::cout << ival << std::endl;  // 0
    return 0;
}
```""",
    "2.6": """```cpp
#include <iostream>
#include <string>
struct Sales_data {
    std::string bookNo;
    unsigned units_sold = 0;
    double revenue = 0.0;
};
int main() {
    Sales_data data1, data2{"978-0-201-78345-0", 25, 22.5};
    std::cout << data2.bookNo << std::endl;
    return 0;
}
```""",
    "2.7": """```cpp
#include <iostream>
int main() {
    // 第 2 章回顾：引用与指针
    int x = 10;
    int &rx = x;
    int *px = &x;
    rx = 20;
    std::cout << *px << std::endl;
    return 0;
}
```""",
    "3.1": """```cpp
#include <iostream>
#include <string>
using std::cin;
using std::cout;
using std::endl;
using std::string;
int main() {
    string line;
    getline(cin, line);
    cout << line << endl;
    return 0;
}
```""",
    "3.2": """```cpp
#include <iostream>
#include <string>
int main() {
    std::string s1 = "hello", s2;
    s2 = s1;
    s1 = "world";
    std::cout << s1 << " " << s2 << std::endl;  // world hello
    return 0;
}
```""",
    "3.3": """```cpp
#include <iostream>
#include <vector>
int main() {
    std::vector<int> v{1, 2, 3, 4, 5};
    for (int x : v)
        std::cout << x << " ";
    std::cout << std::endl;
    return 0;
}
```""",
    "3.4": """```cpp
#include <iostream>
#include <vector>
int main() {
    std::vector<int> v{1, 2, 3};
    for (auto it = v.cbegin(); it != v.cend(); ++it)
        std::cout << *it << " ";
    std::cout << std::endl;
    return 0;
}
```""",
    "3.5": """```cpp
#include <iostream>
int main() {
    int arr[3] = {1, 2, 3};
    int *beg = std::begin(arr);
    int *end = std::end(arr);
    for (auto p = beg; p != end; ++p)
        std::cout << *p << " ";
    std::cout << std::endl;
    return 0;
}
```""",
    "3.6": """```cpp
#include <iostream>
int main() {
    int ia[3][4] = {
        {0, 1, 2, 3},
        {4, 5, 6, 7},
        {8, 9, 10, 11}
    };
    for (const auto &row : ia)
        for (int col : row)
            std::cout << col << " ";
    std::cout << std::endl;
    return 0;
}
```""",
    "3.7": """```cpp
#include <iostream>
#include <string>
#include <vector>
int main() {
    std::vector<std::string> words{"C++", "Primer"};
    for (auto &w : words)
        std::cout << w << " ";
    std::cout << std::endl;
    return 0;
}
```""",
    "4.1": """```cpp
#include <iostream>
int main() {
    int i = 3, j = 4;
    std::cout << i + j << " " << i * j << std::endl;
    return 0;
}
```""",
    "4.2": """```cpp
#include <iostream>
int main() {
    int i = 10;
    i += 2;   // 复合赋值
    ++i;      // 前置递增
    int j = i++;  // 后置递增
    std::cout << i << " " << j << std::endl;
    return 0;
}
```""",
    "4.11": """```cpp
#include <iostream>
int main() {
    double d = 3.14;
    int i = static_cast<int>(d);  // 显式类型转换
    std::cout << i << std::endl;
    return 0;
}
```""",
    "4.12": """```cpp
#include <iostream>
int main() {
    // 优先级：* 高于 +
    int a = 2, b = 3;
    std::cout << a + b * 4 << std::endl;  // 14，不是 20
    return 0;
}
```""",
    "5.1": """```cpp
#include <iostream>
int main() {
    int sum = 0;
    for (int i = 1; i <= 10; ++i)
        sum += i;
    std::cout << sum << std::endl;
    return 0;
}
```""",
    "5.3": """```cpp
#include <iostream>
int main() {
    int score = 85;
    if (score >= 90)
        std::cout << "A" << std::endl;
    else if (score >= 60)
        std::cout << "Pass" << std::endl;
    else
        std::cout << "Fail" << std::endl;
    return 0;
}
```""",
    "5.4": """```cpp
#include <iostream>
int main() {
    for (int i = 0; i != 5; ++i)
        std::cout << i << " ";
    std::cout << std::endl;
    int j = 0;
    while (j < 5) {
        std::cout << j << " ";
        ++j;
    }
    std::cout << std::endl;
    return 0;
}
```""",
    "5.5": """```cpp
#include <iostream>
int main() {
    for (int i = 0; i < 10; ++i) {
        if (i == 5)
            break;
        if (i % 2 == 0)
            continue;
        std::cout << i << " ";
    }
    std::cout << std::endl;
    return 0;
}
```""",
    "5.6": """```cpp
#include <iostream>
#include <stdexcept>
double divide(double a, double b) {
    if (b == 0)
        throw std::runtime_error("division by zero");
    return a / b;
}
int main() {
    try {
        std::cout << divide(10, 2) << std::endl;
        std::cout << divide(10, 0) << std::endl;
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
```""",
    "6.1": """```cpp
#include <iostream>
int fact(int n) {
    return n <= 1 ? 1 : n * fact(n - 1);
}
int main() {
    std::cout << fact(5) << std::endl;  // 120
    return 0;
}
```""",
    "6.2": """```cpp
#include <iostream>
void print(const int *beg, const int *end) {
    while (beg != end)
        std::cout << *beg++ << " ";
}
int main() {
    int arr[] = {1, 2, 3};
    print(std::begin(arr), std::end(arr));
    std::cout << std::endl;
    return 0;
}
```""",
    "6.3": """```cpp
#include <iostream>
void print(int i) { std::cout << "int: " << i << std::endl; }
void print(double d) { std::cout << "double: " << d << std::endl; }
int main() {
    print(42);
    print(3.14);
    return 0;
}
```""",
    "6.4": """```cpp
#include <iostream>
constexpr int size() { return 42; }
int main() {
    int arr[size()];  // 编译期常量作数组大小
    std::cout << sizeof(arr) / sizeof(arr[0]) << std::endl;
    return 0;
}
```""",
    "6.5": """```cpp
#include <iostream>
int &get(int *arr, int i) { return arr[i]; }
int main() {
    int ia[3] = {0, 1, 2};
    get(ia, 1) = 42;
    std::cout << ia[1] << std::endl;
    return 0;
}
```""",
    "7.1": """```cpp
#include <iostream>
#include <string>
class Sales_data {
public:
    std::string isbn;
    unsigned units_sold = 0;
    double revenue = 0.0;
    double avg_price() const {
        return units_sold ? revenue / units_sold : 0;
    }
};
int main() {
    Sales_data item;
    item.isbn = "978-0-201-78345-0";
    item.units_sold = 10;
    item.revenue = 250.0;
    std::cout << item.avg_price() << std::endl;
    return 0;
}
```""",
    "7.2": """```cpp
#include <iostream>
class Account {
public:
    void deposit(double amount) { balance += amount; }
    double balance() const { return balance_; }
private:
    double balance_ = 0;
};
int main() {
    Account acc;
    acc.deposit(100);
    std::cout << acc.balance() << std::endl;
    return 0;
}
```""",
    "7.3": """```cpp
#include <iostream>
class Point {
    friend std::ostream &operator<<(std::ostream &os, const Point &p);
public:
    Point(int x, int y) : x_(x), y_(y) {}
private:
    int x_, y_;
};
std::ostream &operator<<(std::ostream &os, const Point &p) {
    return os << "(" << p.x_ << "," << p.y_ << ")";
}
int main() {
    Point p(3, 4);
    std::cout << p << std::endl;
    return 0;
}
```""",
    "7.4": """```cpp
#include <iostream>
class X {
public:
    X() { std::cout << "default ctor\\n"; }
    X(const X &) { std::cout << "copy ctor\\n"; }
};
int main() {
    X a;
    X b = a;
    return 0;
}
```""",
    "7.5": """```cpp
#include <iostream>
#include <string>
class Person {
public:
    Person(std::string n, int a) : name_(std::move(n)), age_(a) {}
    void print() const {
        std::cout << name_ << ", " << age_ << std::endl;
    }
private:
    std::string name_;
    int age_;
};
int main() {
    Person p("Alice", 30);
    p.print();
    return 0;
}
```""",
    "7.6": """```cpp
#include <iostream>
class Account {
public:
    static int object_count() { return count_; }
    Account() { ++count_; }
private:
    static int count_;
};
int Account::count_ = 0;
int main() {
    Account a, b;
    std::cout << Account::object_count() << std::endl;
    return 0;
}
```""",
    "8.1": """```cpp
#include <iostream>
int main() {
    int ival;
    double dval;
    std::cin >> ival >> dval;
    std::cout << ival << " " << dval << std::endl;
    return 0;
}
```""",
    "8.2": """```cpp
#include <iostream>
int main() {
    int ival;
    while (std::cin >> ival)
        std::cout << ival << std::endl;
    if (std::cin.eof())
        std::cout << "EOF reached" << std::endl;
    return 0;
}
```""",
    "8.3": """```cpp
#include <fstream>
#include <iostream>
#include <string>
int main() {
    std::ofstream out("out.txt");
    out << "Hello file" << std::endl;
    std::ifstream in("out.txt");
    std::string line;
    std::getline(in, line);
    std::cout << line << std::endl;
    return 0;
}
```""",
    "8.4": """```cpp
#include <sstream>
#include <iostream>
#include <string>
int main() {
    std::istringstream iss("42 3.14");
    int i; double d;
    iss >> i >> d;
    std::ostringstream oss;
    oss << "i=" << i << ", d=" << d;
    std::cout << oss.str() << std::endl;
    return 0;
}
```""",
    "9.1": """```cpp
#include <iostream>
#include <vector>
int main() {
    std::vector<int> v = {1, 2, 3};
    v.push_back(4);
    for (auto x : v)
        std::cout << x << " ";
    std::cout << std::endl;
    return 0;
}
```""",
    "9.2": """```cpp
#include <iostream>
#include <vector>
int main() {
    std::vector<int> v(10, 42);  // 10 个 42
    std::cout << v.size() << " " << v[0] << std::endl;
    return 0;
}
```""",
    "9.3": """```cpp
#include <iostream>
#include <string>
int main() {
    std::string s = "hello";
    s += " world";
    std::cout << s.substr(0, 5) << std::endl;
    return 0;
}
```""",
    "9.4": """```cpp
#include <iostream>
#include <stack>
#include <queue>
int main() {
    std::stack<int> stk;
    stk.push(1); stk.push(2);
    std::cout << stk.top() << std::endl;
    std::queue<int> q;
    q.push(1); q.push(2);
    std::cout << q.front() << std::endl;
    return 0;
}
```""",
    "10.1": """```cpp
#include <algorithm>
#include <iostream>
#include <vector>
int main() {
    std::vector<int> v{3, 1, 4, 1, 5};
    std::sort(v.begin(), v.end());
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
    return 0;
}
```""",
    "10.2": """```cpp
#include <algorithm>
#include <iostream>
#include <vector>
int main() {
    std::vector<int> v{1, 2, 3, 4, 5};
    auto it = std::find_if(v.begin(), v.end(),
        [](int n) { return n > 3; });
    if (it != v.end())
        std::cout << *it << std::endl;
    return 0;
}
```""",
    "10.3": """```cpp
#include <iostream>
#include <vector>
int main() {
    std::vector<int> v{1, 2, 3, 4, 5};
    auto mid = v.begin() + v.size() / 2;
    std::cout << *mid << std::endl;
    return 0;
}
```""",
    "10.4": """```cpp
#include <algorithm>
#include <iostream>
#include <vector>
int main() {
    std::vector<int> v{5, 4, 3, 2, 1};
    std::sort(v.begin(), v.end(), std::greater<int>());
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
    return 0;
}
```""",
    "11.1": """```cpp
#include <iostream>
#include <map>
int main() {
    std::map<std::string, int> word_count;
    ++word_count["hello"];
    ++word_count["world"];
    std::cout << word_count["hello"] << std::endl;
    return 0;
}
```""",
    "11.2": """```cpp
#include <iostream>
#include <utility>
int main() {
    std::pair<std::string, int> p{"key", 42};
    std::cout << p.first << ": " << p.second << std::endl;
    return 0;
}
```""",
    "11.3": """```cpp
#include <iostream>
#include <set>
int main() {
    std::set<int> s{3, 1, 4, 1, 5};
    auto it = s.lower_bound(3);
    std::cout << *it << std::endl;
    return 0;
}
```""",
    "12.1": """```cpp
#include <iostream>
#include <memory>
int main() {
    std::shared_ptr<int> p1 = std::make_shared<int>(42);
    std::shared_ptr<int> p2 = p1;
    std::cout << *p1 << " use_count=" << p1.use_count() << std::endl;
    return 0;
}
```""",
    "12.2": """```cpp
#include <iostream>
int main() {
    int *p = new int(42);
    std::cout << *p << std::endl;
    delete p;
    p = nullptr;
    return 0;
}
```""",
    "12.3": """```cpp
#include <iostream>
#include <memory>
int main() {
    std::unique_ptr<int[]> up(new int[5]{1, 2, 3, 4, 5});
    for (int i = 0; i < 5; ++i)
        std::cout << up[i] << " ";
    std::cout << std::endl;
    return 0;
}
```""",
    "12.4": """```cpp
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
int main() {
    std::map<std::string, std::set<unsigned>> dict;
  dict["apple"].insert(1);
  dict["apple"].insert(3);
    std::cout << dict["apple"].size() << std::endl;
    return 0;
}
```""",
    "13.1": """```cpp
#include <iostream>
#include <string>
class HasPtr {
public:
    HasPtr(const std::string &s) : ps(new std::string(s)) {}
    HasPtr(const HasPtr &p) : ps(new std::string(*p.ps)) {}
    HasPtr &operator=(const HasPtr &);
    ~HasPtr() { delete ps; }
    std::string &str() { return *ps; }
private:
    std::string *ps;
};
HasPtr &HasPtr::operator=(const HasPtr &rhs) {
    auto newp = new std::string(*rhs.ps);
    delete ps;
    ps = newp;
    return *this;
}
int main() {
    HasPtr a("hi"), b = a;
    std::cout << a.str() << " " << b.str() << std::endl;
    return 0;
}
```""",
    "13.2": """```cpp
#include <iostream>
#include <memory>
class Blob {
public:
    void push_back(int i) { data->push_back(i); }
private:
    std::shared_ptr<std::vector<int>> data =
        std::make_shared<std::vector<int>>();
};
int main() {
    Blob b;
    b.push_back(42);
    return 0;
}
```""",
    "13.3": """```cpp
#include <iostream>
#include <vector>
int main() {
    std::vector<int> v1{1, 2, 3};
    std::vector<int> v2 = std::move(v1);
    std::cout << v2.size() << " " << v1.size() << std::endl;
    return 0;
}
```""",
    "13.4": """```cpp
#include <iostream>
#include <string>
class String {
public:
    String(const char *s) : data_(s ? s : "") {}
    String(String &&other) noexcept : data_(std::move(other.data_)) {}
    const std::string &str() const { return data_; }
private:
    std::string data_;
};
int main() {
    String s("hello");
    String t(std::move(s));
    std::cout << t.str() << std::endl;
    return 0;
}
```""",
    "14.1": """```cpp
#include <iostream>
class Complex {
    friend std::ostream &operator<<(std::ostream &, const Complex &);
public:
    Complex(double r, double i) : re(r), im(i) {}
private:
    double re, im;
};
std::ostream &operator<<(std::ostream &os, const Complex &c) {
    return os << c.re << "+" << c.im << "i";
}
int main() {
    Complex c(1, 2);
    std::cout << c << std::endl;
    return 0;
}
```""",
    "14.2": """```cpp
#include <iostream>
class Counter {
public:
    Counter &operator++() { ++val_; return *this; }
    int operator++(int) { int old = val_; ++val_; return old; }
    int value() const { return val_; }
private:
    int val_ = 0;
};
int main() {
    Counter c;
    std::cout << ++c << " " << c++ << " " << c.value() << std::endl;
    return 0;
}
```""",
    "14.3": """```cpp
#include <iostream>
class Fraction {
public:
    explicit Fraction(int n) : num_(n), den_(1) {}
    int numerator() const { return num_; }
private:
    int num_, den_;
};
int main() {
    Fraction f(3);
    // Fraction g = 3;  // 错误：explicit 禁止隐式转换
    Fraction g(3);
    std::cout << g.numerator() << std::endl;
    return 0;
}
```""",
    "15.1": """```cpp
#include <iostream>
class Quote {
public:
    virtual double net_price(std::size_t cnt) const {
        return cnt >= bulk ? price * (1 - disc) * cnt : price * cnt;
    }
    virtual ~Quote() = default;
protected:
    double price = 0.0;
    std::size_t bulk = 0;
    double disc = 0.0;
};
int main() {
    Quote q;
    std::cout << q.net_price(10) << std::endl;
    return 0;
}
```""",
    "15.2": """```cpp
#include <iostream>
class Base {
public:
    virtual void print() const { std::cout << "Base\\n"; }
    virtual ~Base() = default;
};
class Derived : public Base {
public:
    void print() const override { std::cout << "Derived\\n"; }
};
int main() {
    Derived d;
    Base *bp = &d;
    bp->print();  // Derived
    return 0;
}
```""",
    "15.3": """```cpp
#include <iostream>
class Instrument {
public:
    virtual void play() const = 0;  // 纯虚函数
    virtual ~Instrument() = default;
};
class Piano : public Instrument {
public:
    void play() const override { std::cout << "Piano\\n"; }
};
int main() {
    Piano p;
    p.play();
    return 0;
}
```""",
    "15.4": """```cpp
#include <iostream>
class Base {
public:
    virtual ~Base() { std::cout << "~Base\\n"; }
};
class Derived : public Base {
public:
    ~Derived() { std::cout << "~Derived\\n"; }
};
int main() {
    Base *p = new Derived;
    delete p;  // 虚析构保证先 ~Derived 再 ~Base
    return 0;
}
```""",
    "15.5": """```cpp
#include <iostream>
class A { public: void f() { std::cout << "A\\n"; } };
class B : public A {};
class C : public A {};
class D : public B, public C {};  // 菱形继承
int main() {
    D d;
    d.B::f();
    d.C::f();
    return 0;
}
```""",
    "16.1": """```cpp
#include <iostream>
template<typename T>
T maximum(const T &a, const T &b) {
    return a > b ? a : b;
}
int main() {
    std::cout << maximum(3, 7) << std::endl;
    std::cout << maximum(3.14, 2.71) << std::endl;
    return 0;
}
```""",
    "16.2": """```cpp
#include <iostream>
template<typename T>
class Stack {
public:
    void push(const T &v) { data_[top_++] = v; }
    T pop() { return data_[--top_]; }
private:
    T data_[100];
    int top_ = 0;
};
int main() {
    Stack<int> s;
    s.push(1); s.push(2);
    std::cout << s.pop() << std::endl;
    return 0;
}
```""",
    "16.3": """```cpp
#include <iostream>
#include <vector>
template<typename C>
void print(const C &c) {
    for (const auto &e : c)
        std::cout << e << " ";
    std::cout << std::endl;
}
int main() {
    std::vector<int> v{1, 2, 3};
    print(v);
    return 0;
}
```""",
    "16.4": """```cpp
#include <iostream>
template<typename T>
void foo(const T &) {}
template<>
void foo<int>(const int &) {
    std::cout << "int specialization\\n";
}
int main() {
    foo(3.14);
    foo(42);
    return 0;
}
```""",
    "17.1": """```cpp
#include <iostream>
#include <tuple>
#include <string>
int main() {
    std::tuple<std::string, int, double> t{"book", 10, 9.9};
    std::cout << std::get<0>(t) << " "
              << std::get<1>(t) << std::endl;
    return 0;
}
```""",
    "17.2": """```cpp
#include <bitset>
#include <iostream>
int main() {
    std::bitset<8> bs("10101010");
    std::cout << bs.count() << std::endl;
    bs.set(0);
    std::cout << bs << std::endl;
    return 0;
}
```""",
    "17.3": """```cpp
#include <iostream>
#include <regex>
#include <string>
int main() {
    std::regex pat(R"(\\d+)");
    std::string s = "order 42 items";
    std::smatch m;
    if (std::regex_search(s, m, pat))
        std::cout << m.str() << std::endl;
    return 0;
}
```""",
    "17.4": """```cpp
#include <iostream>
#include <random>
int main() {
    std::default_random_engine eng;
    std::uniform_int_distribution<int> dist(1, 6);
    for (int i = 0; i < 5; ++i)
        std::cout << dist(eng) << " ";
    std::cout << std::endl;
    return 0;
}
```""",
    "17.5": """```cpp
#include <chrono>
#include <iostream>
#include <thread>
using namespace std::chrono;
int main() {
    auto start = steady_clock::now();
    std::this_thread::sleep_for(milliseconds(100));
    auto end = steady_clock::now();
    auto ms = duration_cast<milliseconds>(end - start).count();
    std::cout << ms << " ms\\n";
    return 0;
}
```""",
    "17.6": """```cpp
#include <iostream>
#include <optional>
int main() {
    std::optional<int> o;
    o = 42;
    if (o)
        std::cout << *o << std::endl;
    o.reset();
    std::cout << o.value_or(-1) << std::endl;
    return 0;
}
```""",
    "17.7": """```cpp
#include <iostream>
// 第 17 章回顾：tuple + chrono
#include <chrono>
#include <tuple>
int main() {
    auto t = std::make_tuple("done", std::chrono::seconds(1));
    std::cout << std::get<0>(t) << std::endl;
    return 0;
}
```""",
    "18.1": """```cpp
#include <iostream>
#include <stdexcept>
double safe_divide(double a, double b) {
    if (b == 0) throw std::invalid_argument("b is zero");
    return a / b;
}
int main() {
    try {
        std::cout << safe_divide(10, 2) << std::endl;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
```""",
    "18.2": """```cpp
#include <iostream>
namespace utils {
    void hello() { std::cout << "hello from utils\\n"; }
}
int main() {
    utils::hello();
    using utils::hello;
    hello();
    return 0;
}
```""",
    "18.3": """```cpp
#include <iostream>
class Base {
public:
    virtual void f() { std::cout << "Base\\n"; }
};
class D1 : public Base {};
class D2 : public Base {};
class Most : public D1, public D2 {};
int main() {
    Most m;
    m.D1::f();
    return 0;
}
```""",
    "18.4": """```cpp
#include <iostream>
#include <typeinfo>
class Base { public: virtual ~Base() = default; };
class Derived : public Base {};
int main() {
    Derived d;
    Base &b = d;
    std::cout << typeid(b).name() << std::endl;
    if (auto *p = dynamic_cast<Derived *>(&b))
        std::cout << "ok\\n";
    return 0;
}
```""",
    "18.5": """```cpp
#include <iostream>
// 第 18 章回顾：异常 + 命名空间
namespace app {
    void run() { std::cout << "running\\n"; }
}
int main() {
    try { app::run(); }
    catch (...) { std::cerr << "error\\n"; }
    return 0;
}
```""",
    "19.1": """```cpp
#include <iostream>
void *operator new(std::size_t sz) {
    std::cout << "alloc " << sz << " bytes\\n";
    return ::operator new(sz);
}
int main() {
    int *p = new int(42);
    delete p;
    return 0;
}
```""",
    "19.2": """```cpp
#include <iostream>
enum class Color { Red, Green, Blue };
int main() {
    Color c = Color::Red;
    if (c == Color::Red)
        std::cout << "red\\n";
    return 0;
}
```""",
    "19.3": """```cpp
#include <iostream>
void f(int) { std::cout << "int\\n"; }
void f(int *) { std::cout << "pointer\\n"; }
int main() {
    f(nullptr);  // 调用 f(int*)
    return 0;
}
```""",
    "19.4": """```cpp
#include <iostream>
class Outer {
public:
    class Inner {
    public:
        void show() const { std::cout << "inner\\n"; }
    };
};
int main() {
    Outer::Inner i;
    i.show();
    return 0;
}
```""",
    "19.5": """```cpp
#include <iostream>
union Data {
    int i;
    char c[4];
};
struct Flags {
    unsigned ready : 1;
    unsigned mode  : 3;
};
int main() {
    Data d{};
    d.i = 0x41424344;
    Flags f{1, 5};
    std::cout << (int)f.ready << " " << (int)f.mode << std::endl;
    return 0;
}
```""",
    "19.6": """```cpp
#include <iostream>
template<typename... Args>
void print(Args... args) {
    ((std::cout << args << " "), ...);
    std::cout << std::endl;
}
int main() {
    print(1, 2.5, "hi");
    return 0;
}
```""",
    "19.7": """```cpp
#include <iostream>
#include <utility>
void process(int &)  { std::cout << "lvalue\\n"; }
void process(int &&) { std::cout << "rvalue\\n"; }
template<typename T>
void forward_call(T &&arg) {
    process(std::forward<T>(arg));
}
int main() {
    int x = 1;
    forward_call(x);
    forward_call(2);
    return 0;
}
```""",
    "19.8": """```cpp
#include <iostream>
#include <memory>
int main() {
    alignas(16) char buf[64];
    int *p = new (buf) int(42);  // placement new
    std::cout << *p << std::endl;
    p->~int();
    return 0;
}
```""",
    "19.9": """```cpp
#include <iostream>
// 第 19 章回顾：完美转发 + 可变参模板
#include <utility>
template<typename T>
void wrap(T &&v) {
    std::cout << std::forward<T>(v) << std::endl;
}
int main() {
    wrap(42);
    return 0;
}
```""",
}

EFFECTIVE_EXAMPLES: dict[int, str] = {
    1: """```cpp
// C 子集：数组与指针
char buf[] = "hello";
// OOP：类与封装
class Widget { public: void draw() {} };
// 模板 + STL
#include <vector>
std::vector<int> v{1, 2, 3};
```""",
    2: """```cpp
// 不好：#define ASPECT_RATIO 1.653
const double aspect_ratio = 1.653;
enum class Color { Red, Green, Blue };
inline int max(int a, int b) { return a > b ? a : b; }
```""",
    3: """```cpp
class TextBlock {
public:
    char operator[](std::size_t pos) const { return text[pos]; }
    char &operator[](std::size_t pos) { return text[pos]; }
private:
    std::string text;
};
```""",
    4: """```cpp
class PhoneNumber { /* ... */ };
class Person {
    std::string name;
    PhoneNumber phone;  // 成员自动默认构造，优于裸指针
public:
    Person() : name(""), phone() {}
};
```""",
    5: """```cpp
class Empty {};
class HasMembers {
    int x;
    std::string s;
    // 编译器自动生成：默认构造、拷贝构造、拷贝赋值、析构
};
```""",
    6: """```cpp
class NonCopyable {
public:
    NonCopyable() = default;
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;
};
```""",
    7: """```cpp
class Base { public: virtual ~Base() = default; };
class Derived : public Base {};
void deleteViaBase(Base *p) { delete p; }  // 需要虚析构
```""",
    8: """```cpp
struct Data { ~Data() { /* 可能抛异常 */ } };
void close(Data &d) noexcept {
    try { /* 清理 */ } catch (...) { /* 吞掉，析构路径不传播 */ }
}
```""",
    9: """```cpp
class Base {
public:
    Base() { log(); }  // 不要：调用 virtual log()
    virtual void log() { std::cout << "Base\\n"; }
};
```""",
    10: """```cpp
class Widget {
public:
    Widget &operator=(const Widget &rhs) {
        // ...
        return *this;
    }
};
```""",
    11: """```cpp
Widget &Widget::operator=(const Widget &rhs) {
    if (this == &rhs) return *this;
    // 释放旧资源，复制新资源
    return *this;
}
```""",
    12: """```cpp
class String {
    char *data;
public:
    String(const String &o) : data(new char[std::strlen(o.data) + 1]) {
        std::strcpy(data, o.data);
    }
};
```""",
    13: """```cpp
class Lock {
    std::mutex &m;
public:
    explicit Lock(std::mutex &mu) : m(mu) { m.lock(); }
    ~Lock() { m.unlock(); }
};
```""",
    14: """```cpp
class Handle {
    std::shared_ptr<Resource> res;
public:
    Handle(const Handle &) = default;  // 或禁止拷贝
    Resource *get() const { return res.get(); }
};
```""",
    15: """```cpp
class Font {
    std::shared_ptr<FontHandle> fh;
public:
    FontHandle *get() const { return fh.get(); }
    FontHandle &operator*() const { return *fh; }
};
```""",
    16: """```cpp
std::unique_ptr<int> p(new int(42));  // 好
// std::shared_ptr<int> sp(new int(42), new int(43));  // 坏：两条 new
```""",
    17: """```cpp
std::shared_ptr<Widget> pw(new Widget);
processWidget(pw, priority());  // 用独立语句，避免异常泄漏
```""",
    18: """```cpp
class Date {
public:
    explicit Date(int y, int m, int d);
    static Date fromString(const std::string &);  // 工厂函数
};
```""",
    19: """```cpp
class BankAccount {
public:
    void deposit(double amount);  // 接口简单、难误用
    // 不暴露裸指针给外部随意修改余额
};
```""",
    20: """```cpp
void print(const std::string &s);  // 好：避免拷贝
// void print(std::string s);      // 差：每次传参都拷贝
```""",
    21: """```cpp
const Rational operator+(const Rational &a, const Rational &b) {
    return Rational(a.n + b.n, a.d);  // 按值返回对象，不要返回局部引用
}
```""",
    22: """```cpp
class AccessDemo {
public:
    void pub_api();
private:
    int data_;  // 数据成员一律 private
};
```""",
    23: """```cpp
class UDate {
    friend bool checkValidity(const UDate &);
public:
    int month() const;
};
bool checkValidity(const UDate &d) { return d.month() <= 12; }
```""",
    24: """```cpp
class Rational {
    int n, d;
    friend Rational operator*(const Rational &, const Rational &);
};
Rational operator*(const Rational &a, const Rational &b) {
    return Rational(a.n * b.n, a.d * b.d);
}
```""",
    25: """```cpp
namespace std {
    template<> void swap(MyClass &a, MyClass &b) noexcept {
        a.swap(b);  // 不抛异常的 swap 重载
    }
}
```""",
    26: """```cpp
std::string process() {
    if (condition)
        std::string s;  // 需要时才定义，缩小作用域
        // ...
    return result;
}
```""",
    27: """```cpp
void f(Base *bp) {
    if (auto *d = dynamic_cast<Derived *>(bp)) { /* ... */ }
    // 避免 C 风格 (Derived*)bp
}
```""",
    28: """```cpp
class String {
    char *data;
public:
    const char &operator[](int i) const { return data[i]; }
    // 不要返回 char& 的非 const 版本给 const 对象外的句柄
};
```""",
    29: """```cpp
class MutexGuard {
    std::mutex &m;
public:
    MutexGuard(std::mutex &mu) : m(mu) { m.lock(); }
    ~MutexGuard() { m.unlock(); }  // 异常时也能解锁
};
```""",
    30: """```cpp
// Widget.h 只前置声明，实现放 Widget.cpp
class WidgetImpl;
class Widget {
    std::unique_ptr<WidgetImpl> pImpl;
};
```""",
    31: """```cpp
// 头文件尽量少 #include，多用前置声明
class Observer;
class Subject {
    std::vector<std::unique_ptr<Observer>> obs;
};
```""",
    32: """```cpp
class Person { /* 人 */ };
class Student : public Person { /* Student is-a Person */ };
void study(const Person &p) { /* 接受任何人 */ }
```""",
    33: """```cpp
class Shape {
public:
    virtual void draw() = 0;   // 接口继承
    virtual void resize(int) { /* 默认实现 */ }  // 实现继承
};
```""",
    34: """```cpp
class Base { public: void mf() { /* 非虚 */ } };
class Derived : public Base {};
Derived d;
d.mf();  // 不要指望多态地改写非虚 mf
```""",
    35: """```cpp
class Base {
public:
    virtual void f(int x = 0);
};
class Derived : public Base {
public:
    void f(int x = 1) override;  // 缺省参数仍来自 Base::f 的声明
};
```""",
    36: """```cpp
class A { public: void f(); };
class B : public A {};
class C : public A {};
// 用 using A::f 或不同函数名，避免多层遮蔽
```""",
    37: """```cpp
class Base { public: virtual void print(int x = 10) = 0; };
class Derived : public Base {
public:
    void print(int x = 20) override;  // 不要改缺省参数
};
```""",
    38: """```cpp
class Engine { /* ... */ };
class Car {
    Engine engine_;  // 组合优于继承
public:
    void start() { engine_.ignite(); }
};
```""",
    39: """```cpp
class Engine { void tune(); };
class Car : private Engine {};  // 谨慎：实现继承，不是 is-a
```""",
    40: """```cpp
class A {};
class B : public A {};
class C : public A {};
class D : public B, public C {};  // 多重继承需谨慎
```""",
    41: """```cpp
template<typename Iter>
void doAdvance(Iter &it, int n) {
    it += n;  // 隐式接口：Iter 必须支持 +=
}
```""",
    42: """```cpp
template<typename T>
void f(T x) { /* T 可以是 typename 或 class，此处等价 */ }
```""",
    43: """```cpp
template<typename T>
class Base { public: void mf(); };
template<typename T>
class Derived : public Base<T> {
    void g() { this->mf(); }  // 或 Base<T>::mf()
};
```""",
    44: """```cpp
template<typename T>
class SquareMatrixBase {
protected:
    void invert(std::size_t size);  // 与 T 无关的代码抽离
};
template<typename T>
class SquareMatrix : private SquareMatrixBase<T> { /* ... */ };
```""",
    45: """```cpp
template<typename T>
class SmartPtr {
public:
    template<typename U>
    SmartPtr(const SmartPtr<U> &other);  // 接受兼容类型
};
```""",
    46: """```cpp
template<typename T>
class Widget {
    friend void doStuff(const Widget<T> &w) { /* 可访问私有 */ }
};
```""",
    47: """```cpp
template<typename Iter>
struct iterator_traits;  // traits 萃取迭代器类型信息
// std::iterator_traits<Iter>::value_type
```""",
    48: """```cpp
int *p = new int;
delete p;
Widget *w = ::new Widget;
::delete w;
void *buf = ::operator new(64);
::operator delete(buf);
```""",
    49: """```cpp
void *operator new(std::size_t sz) {
    if (void *p = std::malloc(sz)) return p;
    throw std::bad_alloc();
}
void operator delete(void *p) noexcept { std::free(p); }
```""",
    50: """```cpp
void *operator new(std::size_t, void *place) { return place; }
void operator delete(void *, void *) noexcept {}  // placement delete
```""",
    51: """```cpp
// 全局 operator new/delete 影响整个程序，重载前三思
// 通常类专属 new/delete 或内存池更合适
```""",
    52: """```cpp
alignas(std::max_align_t) char buf[sizeof(T)];
T *p = new (buf) T(args);
// ...
p->~T();  // placement new 不自动析构
```""",
    53: """```cpp
#if __cplusplus >= 201103L
    // C++11 特性
#else
    // 回退实现
#endif
```""",
    54: """```cpp
#include <algorithm>
#include <vector>
std::sort(v.begin(), v.end());  // 优先标准库
```""",
    55: """```cpp
// 编码规范、静态分析、单元测试、代码评审
// 示例：简单断言
#include <cassert>
void setAge(int age) { assert(age >= 0); }
```""",
}

MORE_EFFECTIVE_EXAMPLES: dict[int, str] = {
    1: """```cpp
int x = 42;
int &rx = x;   // 引用必须绑定对象
int *px = &x;  // 指针可为空
// void f(int &r); f(*px);  // 错误：不能 *px 当初始化引用的对象链
```""",
    2: """```cpp
double d = 3.14;
int i = static_cast<int>(d);
// int j = (int)d;  // 避免 C 风格强转
```""",
    3: """```cpp
class Base { public: virtual ~Base() = default; };
class Derived : public Base {};
// Base arr[10]; arr[1] = Derived();  // 危险：不要对数组做多态
```""",
    4: """```cpp
class NoDefault {
    int id;
public:
    explicit NoDefault(int i) : id(i) {}  // 无意义默认构造时显式要求参数
};
```""",
    5: """```cpp
class Fraction {
public:
    explicit Fraction(int n) : num_(n) {}
    // Fraction(int) 作为转换函数时要谨慎
private:
    int num_;
};
```""",
    6: """```cpp
class Counter {
    int v_;
public:
    Counter &operator++() { ++v_; return *this; }      // 前置，少一次拷贝
    Counter operator++(int) { Counter t = *this; ++v_; return t; }
};
```""",
    7: """```cpp
if (a && b()) { /* ... */ }  // 不要重载 &&，无法短路
// 同理不要重载 || 和 ,
```""",
    8: """```cpp
void *p1 = ::operator new(100);       // 全局
void *p2 = ::operator new(100, buf);  // placement
Widget *w = new Widget;               // 类专属（若定义）
```""",
    9: """```cpp
class Guard {
    Resource *r;
public:
    ~Guard() { delete r; }  // RAII：析构释放资源
};
```""",
    10: """```cpp
class Widget {
    int *data;
public:
    Widget() : data(new int[100]) {
        if (fail()) throw std::runtime_error("ctor failed");
        // 已分配内存需在 catch 或成员析构中清理
    }
    ~Widget() { delete[] data; }
};
```""",
    11: """```cpp
void acquire() {
    Resource *r = getResource();
    try {
        doWork(r);
    } catch (...) {
        release(r);
        throw;
    }
    release(r);
}
```""",
    12: """```cpp
class MyError : public std::exception {};
void f() {
    try { throw MyError(); }
    catch (MyError e) { /* 按值捕获会切片/拷贝 */ }
}
```""",
    13: """```cpp
try {
    risky();
} catch (...) {
    log("unknown error");
    throw;  // 谨慎重新抛出
}
```""",
    14: """```cpp
void old_api() throw();       // C++17 前异常规范，已不推荐
void modern_api() noexcept;   // 现代写法
```""",
    15: """```cpp
// 异常有展开栈成本；热路径可用错误码
bool parse(const char *s, int &out) noexcept;
```""",
    16: """```cpp
std::string makeName() {
    std::string s = "temp";  // 临时对象
    return s;                // NRVO 可能消除拷贝
}
```""",
    17: """```cpp
// new 涉及堆分配与簿记开销
std::vector<int> v;  // 连续内存，少次 new
v.reserve(1000);
```""",
    18: """```cpp
class Pool {
    char slab[4096];
public:
    void *allocate(std::size_t n);
    void deallocate(void *p);
};
```""",
    19: """```cpp
BigObject factory() {
    return BigObject();  // 返回值优化 RVO
}
BigObject o = factory();
```""",
    20: """```cpp
class Base { public: virtual void f(); };
class Derived : public Base {};
void use(Base &b) { b.f(); }  // 需要多态才用 virtual
```""",
    21: """```cpp
class Base {
public:
    virtual void interface() = 0;
    void helper() { /* 非虚即可 */ }
};
```""",
    22: """```cpp
class Interface {
public:
    virtual void draw() = 0;        // 接口
    virtual void resize(int) { }    // 可选默认实现
};
```""",
    23: """```cpp
void process(Base &b) {
    // Derived *d = static_cast<Derived*>(&b);  // 避免向下转型
    b.virtualMethod();
}
```""",
    24: """```cpp
class A { virtual void f(); };
class B : public A { void f() override; };
// 多重继承时注意虚表与对象布局
```""",
    25: """```cpp
class Base {};
class A : virtual public Base {};
class B : virtual public Base {};
class C : public A, public B {};  // 虚拟继承解决菱形，但有开销
```""",
    26: """```cpp
class HeapOnly {
public:
    static HeapOnly *create() { return new HeapOnly; }
    void destroy() { delete this; }
private:
    HeapOnly() = default;
    ~HeapOnly() = default;
};
```""",
    27: """```cpp
Base *bp = getObject();
if (auto *dp = dynamic_cast<Derived *>(bp)) {
    dp->derivedOnly();
}
```""",
    28: """```cpp
template<typename T>
class SmartPtr {
    T *ptr;
public:
    explicit SmartPtr(T *p) : ptr(p) {}
    ~SmartPtr() { delete ptr; }
    T &operator*() { return *ptr; }
};
```""",
    29: """```cpp
class RefCounted {
    int *count;
    int *data;
public:
    RefCounted() : count(new int(1)), data(new int(0)) {}
    RefCounted(const RefCounted &o) : count(o.count), data(o.data) { ++*count; }
    ~RefCounted() { if (--*count == 0) { delete count; delete data; } }
};
```""",
    30: """```cpp
class Proxy {
    std::vector<int> &vec;
    std::size_t idx;
public:
    Proxy(std::vector<int> &v, std::size_t i) : vec(v), idx(i) {}
    int &operator*() { return vec[idx]; }
};
```""",
    31: """```cpp
class Shape { public: virtual void collide(Shape &) = 0; };
class Rect : public Shape {
public:
    void collide(Shape &other) override {
        other.collideWithRect(*this);  // 双重分派模式
    }
};
```""",
    32: """```cpp
class Plugin {
public:
    virtual ~Plugin() = default;
    virtual int version() const = 0;  // 预留扩展点
};
```""",
    33: """```cpp
class NonLeaf {
public:
    virtual void mustImplement() = 0;
protected:
    NonLeaf() = default;  // 抽象类，不能实例化
};
```""",
    34: """```cpp
extern "C" void c_api(int x);
// C++ 实现
extern "C" void c_api(int x) { /* ... */ }
```""",
    35: """```cpp
#if __cplusplus >= 202002L
    // C++20 特性
#else
    // 兼容旧标准
#endif
```""",
}


def section_id_from_name(name: str) -> str | None:
    m = re.match(r"^([\d]+(?:\.[\d]+)?)", name)
    return m.group(1) if m else None


def item_num_from_name(name: str) -> int | None:
    m = re.match(r"^item(\d+)", name, re.I)
    return int(m.group(1)) if m else None


def get_example(path: Path) -> str:
    name = path.name
    rel = path.relative_to(ROOT).as_posix()

    if "01-C++Primer" in rel:
        sid = section_id_from_name(name)
        if sid and sid in PRIMER_EXAMPLES:
            return PRIMER_EXAMPLES[sid]
        return f"""```cpp
// {path.stem} — 见书中对应小节练习
#include <iostream>
int main() {{
    std::cout << "section {sid or '?'}\\n";
    return 0;
}}
```"""

    if "02-Effective-C++" in rel:
        n = item_num_from_name(name)
        if n and n in EFFECTIVE_EXAMPLES:
            return EFFECTIVE_EXAMPLES[n]
        return f"""```cpp
// Effective C++ 条款 {n or '?'}
#include <iostream>
int main() {{ return 0; }}
```"""

    if "03-More-Effective-C++" in rel:
        n = item_num_from_name(name)
        if n and n in MORE_EFFECTIVE_EXAMPLES:
            return MORE_EFFECTIVE_EXAMPLES[n]
        return f"""```cpp
// More Effective C++ 条款 {n or '?'}
#include <iostream>
int main() {{ return 0; }}
```"""

    return """```cpp
#include <iostream>
int main() { return 0; }
```"""


def should_process(path: Path) -> bool:
    if path.name == "README.md":
        return False
    if "-" not in path.name:
        return False
    return True


def has_example(content: str) -> bool:
    return "## 示例" in content


def append_example(path: Path, dry_run: bool = False) -> bool:
    content = path.read_text(encoding="utf-8")
    if has_example(content):
        return False
    example = get_example(path)
    new_content = content.rstrip() + "\n\n## 示例\n\n" + example + "\n"
    if not dry_run:
        path.write_text(new_content, encoding="utf-8")
    return True


def cleanup_numeric_duplicates(book: Path, dry_run: bool = False) -> int:
    removed = 0
    for ch in book.iterdir():
        if not ch.is_dir():
            continue
        for p in ch.glob("*.md"):
            if p.name == "README.md" or "-" in p.name:
                continue
            sid = section_id_from_name(p.name)
            if not sid:
                continue
            # 存在带中文标题的同名小节则删除纯数字文件
            matches = list(ch.glob(f"{sid}-*.md"))
            if matches:
                if not dry_run:
                    p.unlink()
                removed += 1
    return removed


def main() -> None:
    updated = 0
    skipped = 0
    removed = 0
    for book in BOOKS:
        removed += cleanup_numeric_duplicates(book)
        for path in sorted(book.rglob("*.md")):
            if not should_process(path):
                continue
            if append_example(path):
                updated += 1
            else:
                skipped += 1
    print(f"updated={updated} skipped={skipped} removed_duplicates={removed}")


if __name__ == "__main__":
    main()
