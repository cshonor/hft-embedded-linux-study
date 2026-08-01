# 条款 43：学习处理模板化基类内的名称

## 本节讲什么

派生模板类访问基类模板成员会被编译器遮蔽，`this->`、`using`、显式基类限定三种方案。

## 示例

```cpp
template<typename T>
class Base { public: void mf(); };
template<typename T>
class Derived : public Base<T> {
    void g() { this->mf(); }  // 或 Base<T>::mf()
};
```
