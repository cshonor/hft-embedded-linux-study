# 条款 35：不要重写继承而来的默认参数值

## 本节讲什么

默认参数是编译期绑定，虚函数是运行期绑定；基类指针调用永远走基类默认参数，哪怕重写了派生类参数。

## 示例

```cpp
class Base {
public:
    virtual void f(int x = 0);
};
class Derived : public Base {
public:
    void f(int x = 1) override;  // 缺省参数仍来自 Base::f 的声明
};
```

---

## 代码自测

**题目 1：** 下面代码输出什么？为什么这是坑？
```cpp
class Base {
public:
    virtual void f(int x = 10) {
        std::cout << "Base:" << x;
    }
};
class Derived : public Base {
public:
    void f(int x = 20) override {
        std::cout << "Derived:" << x;
    }
};
Base* p = new Derived;
p->f();  // 输出？
```

<details>
<summary>参考答案</summary>

输出 `Derived:10`。虚函数是动态绑定的（调用 `Derived::f`），但默认参数是静态绑定的（用 `Base` 的默认值 10，因为 `p` 的静态类型是 `Base*`）。这导致 `Derived::f` 收到了「错误」的参数值。规则：绝不在派生类中重新定义继承的默认参数值。C++11 可用 NVI（non-virtual interface）模式避免。

</details>
