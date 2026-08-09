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

---

## 代码自测

**题目 1：** 下面模板代码编译失败，为什么？如何修复？
```cpp
template<typename T>
class Derived : public Base<T> {
    void f() {
        someFunc();  // Base<T>::someFunc() 找不到
    }
};
```

<details>
<summary>参考答案</summary>

模板基类中查找名字时，编译器不会进入依赖基类（`Base<T>`）的作用域——因为 `T` 未定时 `Base<T>` 可能被特化为不含 `someFunc` 的版本。修复：用 `this->` 或 `using` 声明：
```cpp
// 方式1
this->someFunc();
// 方式2
using Base<T>::someFunc;
someFunc();
```

</details>
