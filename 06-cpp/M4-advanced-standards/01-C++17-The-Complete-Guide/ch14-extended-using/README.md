# 第 14 章 扩展 using 声明

**Extended Using Declarations**

## 本章讲什么

C++17 扩展了 `using` 声明：可以变长展开包、可以继承构造函数更灵活、可以给变参基类批量 `using`。

## 要点

### 变长 using

```cpp
// C++17：using 声明可以展开参数包
template <typename... Bases>
struct Derived : Bases... {
    using Bases::operator()...;   // 继承所有基类的 operator()
};

struct A { void operator()(int) {} };
struct B { void operator()(double) {} };

Derived<A, B> d;
d(1);      // 调 A::operator()
d(1.0);    // 调 B::operator()
```

C++14 不能 `using Bases::operator()...`，要逐个写。C++17 的变长 `using` 让"函数对象组合"变简单。

### 继承构造函数的改进

```cpp
struct Base {
    Base(int) {}
    Base(int, int) {}
};

struct Derived : Base {
    using Base::Base;   // 继承所有构造函数
};

Derived d1(1);       // 调 Base(int)
Derived d2(1, 2);    // 调 Base(int, int)
```

C++17 修复了 C++11/14 继承构造函数的一些缺陷（如拷贝省略、初始化列表的传递），行为更符合预期。

### 用途

- **函数对象组合**：多个 lambda/函数对象组合成一个，`using ...::operator()...` 让所有版本可见。
- **策略基类组合**：CRTP 多基类，`using` 批量暴露基类方法。
- **mixin 模式**：`struct Composite : Mixins... { using Mixins::method...; };`

## HFT 关联

- **函数对象组合**：策略的分派逻辑用多个函数对象组合，`using Base::operator()...` 让重载决议正确工作。
- **CRTP mixin**：HFT 工具类用 mixin 组合功能（日志、监控、统计），变长 using 批量暴露。
- **减少样板代码**：不用逐个 `using A::foo; using B::bar;`，一行展开。
- **零开销**：编译期名字引入，运行期无开销。

## 自测题

1. C++17 的变长 `using` 解决了什么问题？C++14 怎么写？
2. `using Bases::operator()...` 的语义是什么？
3. 继承构造函数在 C++17 有什么改进？
4. 函数对象组合如何用变长 using 实现？
5. HFT 的 mixin 模式如何用变长 using 批量暴露方法？
