# 条款 34：不要重写继承而来的非虚成员函数

## 本节讲什么

破坏 is-a 语义，基类指针和派生类指针调用结果不一样，行为割裂。

## 示例

```cpp
class Base { public: void mf() { /* 非虚 */ } };
class Derived : public Base {};
Derived d;
d.mf();  // 不要指望多态地改写非虚 mf
```

---

## 代码自测

**题目 1：** 下面代码输出什么？为什么？
```cpp
class Base {
public:
    void f() { std::cout << "Base"; }  // 非虚
};
class Derived : public Base {
public:
    void f() { std::cout << "Derived"; }
};
Base* p = new Derived;
p->f();  // 输出？
```

<details>
<summary>参考答案</summary>

输出 `Base`。非虚函数是静态绑定的——`p` 的静态类型是 `Base*`，所以调用 `Base::f()`，不受实际对象类型影响。重新定义非虚函数是错误的——如果行为需要因派生类而异，函数应该声明为 virtual。

</details>
