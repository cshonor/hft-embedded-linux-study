# 第二章 构造、析构、赋值运算

共 8 条条款。

## 条款

- [条款 5：了解 C++ 默默编写并调用哪些函数](./item05-了解C++默默编写并调用哪些函数.md)
- [条款 6：不想用编译器自动生成的函数，就明确禁用](./item06-不想用编译器自动生成的函数，就明确禁用.md)
- [条款 7：多态基类声明 virtual 析构函数](./item07-多态基类声明virtual析构函数.md)
- [条款 8：别让异常逃离析构函数](./item08-别让异常逃离析构函数.md)
- [条款 9：绝不在构造和析构过程调用虚函数](./item09-绝不在构造和析构过程调用虚函数.md)
- [条款 10：令 operator= 返回 *this 引用](./item10-令operator=返回this引用.md)
- [条款 11：赋值运算符处理自我赋值](./item11-赋值运算符处理自我赋值.md)
- [条款 12：复制对象时勿忘其每一个成员](./item12-复制对象时勿忘其每一个成员.md)


## 章节摘要

构造、析构与赋值运算：编译器自动生成的函数、禁用不需要的函数、虚析构函数、析构不抛异常、构造/析构不调虚函数、`operator=` 返回 `*this`、处理自我赋值、拷贝勿忘每个成员。

## 代码自测

### Q1: 编译器自动生成哪些函数

```cpp
class Empty {};
// 等价于编译器生成了哪些函数？
Empty e1;
Empty e2(e1);
e2 = e1;
```

> 编译器为 `Empty` 自动生成了哪些函数？

<details>
<summary>答案与复习指引</summary>

**编译器生成 6 个函数（按需）：**
1. 默认构造 `Empty()`
2. 拷贝构造 `Empty(const Empty&)`
3. 拷贝赋值 `Empty& operator=(const Empty&)`
4. 析构 `~Empty()`
5. 移动构造 `Empty(Empty&&)`（C++11，按需）
6. 移动赋值 `Empty& operator=(Empty&&)`（C++11，按需）

**"按需"**：只有当代码用到了某个操作，编译器才生成它。生成了析构会抑制移动操作的自动生成。

**复习：** → [条款 5：了解 C++ 默默编写并调用哪些函数](./item05-了解C++默默编写并调用哪些函数.md)
</details>

### Q2: 虚析构函数

```cpp
class Base {
public:
    ~Base() {}  // 非虚析构
};
class Derived : public Base {
    int *data = new int[100];
public:
    ~Derived() { delete[] data; }
};
Base *p = new Derived;
delete p;
```

> `delete p` 会泄漏吗？为什么？

<details>
<summary>答案与复习指引</summary>

**会泄漏。** `delete p` 调用 `Base::~Base()`（非虚），不调用 `Derived::~Derived()` → `data` 泄漏。

**修复：** `virtual ~Base() {}`

**规则：** 只要类可能被继承且通过基类指针 `delete`，析构必须是 `virtual`。不是 `virtual` 的析构是 OOP 最常见内存泄漏来源。

**反例：** STL 容器/`string` 的析构不是虚的——它们设计为不被继承（虽然技术上可以继承，但不要这么做）。

**复习：** → [条款 7：多态基类声明 virtual 析构函数](./item07-多态基类声明virtual析构函数.md)
</details>

### Q3: 构造中调虚函数

```cpp
class Transaction {
public:
    Transaction() { log(); }  // 构造中调虚函数
    virtual void log() const { std::cout << "base "; }
};
class BuyTransaction : public Transaction {
public:
    void log() const override { std::cout << "buy "; }
};
BuyTransaction b;  // 输出什么？
```

> 输出是 "base" 还是 "buy"？为什么？

<details>
<summary>答案与复习指引</summary>

**输出 "base"。** 构造 `BuyTransaction` 时先构造 `Transaction` 基类——此时对象的 vptr 指向 `Transaction` 的 vtable（派生部分还没构造）。`log()` 调用的是 `Transaction::log()`，不是 `BuyTransaction::log()`。

**原因：** 构造期间虚函数不走派生类版本——因为派生部分尚未构造，调派生类的虚函数会访问未初始化的成员。

**修复：** 把 `log` 改为非虚函数，让派生类在构造时显式传递信息给基类。

**复习：** → [条款 9：绝不在构造和析构过程调用虚函数](./item09-绝不在构造和析构过程调用虚函数.md)
</details>
