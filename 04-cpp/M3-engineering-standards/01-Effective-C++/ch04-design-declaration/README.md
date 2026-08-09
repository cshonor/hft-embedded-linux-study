# 第四章 设计与声明

共 8 条条款。

## 条款

- [条款 18：接口设计要易用正确、难被误用](./item18-接口设计要易用正确、难被误用.md)
- [条款 19：设计类等同于设计一种全新类型](./item19-设计类等同于设计一种全新类型.md)
- [条款 20：优先使用 const 引用传参，而非值传递](./item20-优先使用const引用传参，而非值传递.md)
- [条款 21：必须返回对象时，不要强行返回引用](./item21-必须返回对象时，不要强行返回引用.md)
- [条款 22：成员变量一律声明为 private](./item22-成员变量一律声明为private.md)
- [条款 23：优先用非成员、非友元函数替代成员函数](./item23-优先用非成员、非友元函数替代成员函数.md)
- [条款 24：需要所有参数都支持隐式类型转换时，使用非成员函数](./item24-需要所有参数都支持隐式类型转换时，使用非成员函数.md)
- [条款 25：考虑提供不抛异常的 swap 重载](./item25-考虑提供不抛异常的swap重载.md)


## 章节摘要

设计与声明：易用难误的接口、类设计即类型设计、`const` 引用传参、不强行返回引用、成员变量私有、非成员非友元优于成员、类型转换与非成员函数、不抛异常的 `swap`。

## 代码自测

### Q1: const 引用 vs 值传递

```cpp
class Widget { /* 含 std::string, std::vector */ };
// A: 值传递
void process(Widget w);
// B: const 引用
void process(const Widget &w);
```

> A 和 B 的开销分别是什么？

<details>
<summary>答案与复习指引</summary>

**A（值传递）：** 调用 `Widget` 的拷贝构造——深拷贝所有成员（`string`/`vector`），可能 O(n) 开销。
**B（const 引用）：** 传递引用（指针大小），无拷贝。

**规则：** 自定义类型用 `const T&` 传参；内置类型（`int`/`double`/指针）值传递即可（拷贝开销不大于引用）。

**例外：** `string_view`（C++17）比 `const string&` 更轻（不触发 `string` 构造）。`unique_ptr` 只能移动传递。

**复习：** → [条款 20：优先使用 const 引用传参](./item20-优先使用const引用传参，而非值传递.md)
</details>

### Q2: 返回引用的陷阱

```cpp
class Rational {
    int num, den;
public:
    Rational(int n = 0, int d = 1) : num(n), den(d) {}
    friend const Rational operator*(const Rational& lhs, const Rational& rhs);
};
const Rational operator*(const Rational& lhs, const Rational& rhs) {
    Rational result(lhs.num * rhs.num, lhs.den * rhs.den);
    return result;  // 返回值还是引用？
}
```

> 如果改成返回 `Rational&` 引用会有什么问题？

<details>
<summary>答案与复习指引</summary>

**返回引用的三个灾难：**
1. **返回局部变量的引用** → 悬垂引用（局部变量已销毁）
2. **返回堆上对象的引用** → 谁负责 `delete`？泄漏风险
3. **返回静态变量的引用** → `a * b == c * d` 永远为 `true`（同一静态对象）

**正确做法：** 返回值。编译器通过 RVO/NRVO 消除返回拷贝——`result` 直接在调用者栈上构造，零拷贝。

**复习：** → [条款 21：必须返回对象时，不要强行返回引用](./item21-必须返回对象时，不要强行返回引用.md)
</details>

### Q3: 成员变量私有

```cpp
class AccessLevel {
public:
    int readCount;  // public
private:
    int internalState;
};
```

> 为什么成员变量应该声明为 `private` 而非 `public`？

<details>
<summary>答案与复习指引</summary>

**三个理由：**
1. **语法一致性**：所有访问都通过函数（`getX()`），而非混合 `obj.x` 和 `obj.getX()`
2. **封装**：`private` 允许日后修改内部实现（如加日志/验证/换数据结构），`public` 暴露后改不了
3. **访问控制**：`private` 可以做到只读/只写（getter 没有 setter），`public` 无法限制

**`protected` 也不行：** `protected` 成员变量一旦派生类依赖，同样无法修改——封装程度不比 `public` 好。

**规则：** 成员变量一律 `private`，通过 `public`/`protected` 成员函数提供受控访问。

**复习：** → [条款 22：成员变量一律声明为 private](./item22-成员变量一律声明为private.md)
</details>
