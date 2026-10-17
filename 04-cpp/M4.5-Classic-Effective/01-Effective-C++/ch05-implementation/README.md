# 第五章 实现细节

共 6 条条款。

## 条款

- [条款 26：尽可能延后变量定义](./item26-尽可能延后变量定义.md)
- [条款 27：尽量减少类型转型（cast）](./item27-尽量减少类型转型（cast）.md)
- [条款 28：不要返回指向对象内部成员的句柄（指针/引用）](./item28-不要返回指向对象内部成员的句柄（指针引用）.md)
- [条款 29：编写异常安全的代码](./item29-编写异常安全的代码.md)
- [条款 30：透彻理解内联 inline 的优缺点](./item30-透彻理解内联inline的优缺点.md)
- [条款 31：降低文件之间的编译依赖](./item31-降低文件之间的编译依赖.md)


## 章节摘要

实现细节：延后变量定义、减少类型转换、不返回内部成员句柄、异常安全、inline 优缺点、降低编译依赖（Pimpl）。

## 代码自测

### Q1: 延后变量定义

```cpp
std::string encrypt(const std::string& password);
std::string getPassword() {
    std::string encrypted;  // A: 在这里定义
    if (password.empty()) {
        throw std::logic_error("empty password");
    }
    encrypted = encrypt(password);
    return encrypted;
}
```

> A 行的变量定义有什么问题？如何优化？

<details>
<summary>答案与复习指引</summary>

**问题：** 如果 `password.empty()` 为 `true` 抛异常，`encrypted` 白白构造+析构了一次——浪费。而且后面又用赋值覆盖了默认构造的值（构造+赋值 vs 直接构造）。

**优化：** 延后到有真正初始值时定义：
```cpp
if (password.empty()) throw std::logic_error("empty password");
std::string encrypted(password);  // 直接构造
encrypted = encrypt(encrypted);
return encrypted;
```

**更优：** `return encrypt(password);`（RVO 直接返回）。

**复习：** → [条款 26：尽可能延后变量定义](./item26-尽可能延后变量定义.md)
</details>

### Q2: 返回内部成员句柄

```cpp
class Rectangle {
    Rect r;
public:
    Rect& getRect() { return r; }  // 危险？
    const Rect& getRect() const { return r; }
};
const Rectangle& makeRect() { return Rectangle(); }  // 返回临时对象的 const 引用
auto& inner = makeRect().getRect();  // 合法吗？
```

> `inner` 有效吗？返回内部成员的引用有什么风险？

<details>
<summary>答案与复习指引</summary>

**`inner` 悬垂——UB。** `makeRect()` 返回临时对象的 const 引用，但临时对象在语句结束时销毁。`getRect()` 返回内部 `Rect` 的引用，指向已销毁对象。

**风险：** 返回内部成员的指针/引用破坏了封装——外部可以绕过类的接口修改内部状态。即使返回 `const&`，也可能因对象生命周期问题导致悬垂。

**规则：** 不要返回指向对象内部成员的句柄（指针/引用/迭代器）。如果必须暴露，确保对象生命周期覆盖使用点。

**复习：** → [条款 28：不要返回指向对象内部成员的句柄](./item28-不要返回指向对象内部成员的句柄（指针引用）.md)
</details>
