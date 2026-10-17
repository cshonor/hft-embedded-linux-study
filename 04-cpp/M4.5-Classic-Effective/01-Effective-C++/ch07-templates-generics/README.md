# 第七章 模板与泛型编程

共 7 条条款。

## 条款

- [条款 41：理解隐式接口和编译期多态](./item41-理解隐式接口和编译期多态.md)
- [条款 42：typename 和 class 在模板里的区别](./item42-typename和class在模板里的区别.md)
- [条款 43：学习处理模板化基类内的名称](./item43-学习处理模板化基类内的名称.md)
- [条款 44：把与参数无关的代码抽离模板](./item44-把与参数无关的代码抽离模板.md)
- [条款 45：运用成员函数模板接受所有兼容类型](./item45-运用成员函数模板接受所有兼容类型.md)
- [条款 46：在模板内定义非成员函数，使用友元模板](./item46-在模板内定义非成员函数，使用友元模板.md)
- [条款 47：用 traits 萃取类获取类型信息](./item47-用traits萃取类获取类型信息.md)


## 章节摘要

模板与泛型：隐式接口 vs 显式接口、`typename` 用法、模板化基类名称处理、抽离与参数无关的代码、成员函数模板接受兼容类型、友元模板、traits。

## 代码自测

### Q1: typename 的作用

```cpp
template<typename T>
void print_size(T& container) {
    // T::size_type 是什么？需要 typename 吗？
    T::size_type n = container.size();
    std::cout << n;
}
```

> `T::size_type` 前需要 `typename` 吗？为什么？

<details>
<summary>答案与复习指引</summary>

**需要。** `typename T::size_type n = ...;`

**原因：** 编译器在模板定义时不知道 `T::size_type` 是类型还是变量（取决于 `T` 的具体类型）。C++ 默认假设依赖名字（dependent name）不是类型。`typename` 告诉编译器"这是一个类型"。

**例外：** 在模板参数列表中 `typename` 和 `class` 可互换；在"基类列表"和"成员初始化列表"中不用 `typename`（编译器已知是类型）。

**C++20 简化：** Concepts 可以约束模板参数，减少 `typename` 的需要。

**复习：** → [条款 42：typename 和 class 在模板里的区别](./item42-typename和class在模板里的区别.md)
</details>

### Q2: 模板化基类名称

```cpp
template<typename T>
class Derived : public T {
public:
    void f() {
        // base_func();  // A: T 的成员函数，编译器能找到吗？
        T::base_func();  // B: 显式限定
        this->base_func();  // C: 通过 this
    }
};
```

> A、B、C 分别能编译吗？为什么？

<details>
<summary>答案与复习指引</summary>

- A: **编译失败**——编译器在模板定义时不知道 `T` 有没有 `base_func()`，默认不查找模板化基类的成员
- B: **能编译**——显式限定 `T::base_func()` 假定 `T` 有此函数（如果实例化时没有，编译报错）
- C: **能编译**——`this->base_func()` 让编译器在实例化时查找基类成员

**原因：** 模板化基类（`T` 是模板参数）的成员在模板定义时是"不可见的"——编译器不知道 `T` 是什么，不假设它有任何成员。三种方法让成员可见：`this->`、`T::`、`using T::base_func;`。

**复习：** → [条款 43：学习处理模板化基类内的名称](./item43-学习处理模板化基类内的名称.md)
</details>
