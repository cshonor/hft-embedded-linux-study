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
