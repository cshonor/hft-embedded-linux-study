# 条款 36：避免多层继承里出现名字遮蔽

## 本节讲什么

派生类同名函数会隐藏基类所有重载版本，用 `using` 引入基类重载。

## 示例

```cpp
class A { public: void f(); };
class B : public A {};
class C : public A {};
// 用 using A::f 或不同函数名，避免多层遮蔽
```

---

## 代码自测

**题目 1：** 下面代码中 `Derived::func` 遮蔽了 `Base::func(int)`。如何修复？
```cpp
class Base {
public:
    void func(int x) { /* ... */ }
};
class Derived : public Base {
public:
    void func(double d) { /* ... */ }
};
Derived d;
d.func(42);  // 调用哪个？
```

<details>
<summary>参考答案</summary>

调用 `Derived::func(double)`——`42` 被隐式转为 double。`Base::func(int)` 被名字遮蔽，即使参数类型更匹配也不可用。修复：在 Derived 中使用 using 声明：
```cpp
class Derived : public Base {
public:
    using Base::func;  // 引入基类的重载
    void func(double d) { /* ... */ }
};
```
现在 `d.func(42)` 会调用 `Base::func(int)`，`d.func(3.14)` 调用 `Derived::func(double)`。

</details>
