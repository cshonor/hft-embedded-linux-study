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
