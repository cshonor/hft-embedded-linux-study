# 条款 45：运用成员函数模板接受所有兼容类型

## 本节讲什么

智能指针 `shared_ptr` 构造模板，支持派生类向基类隐式转换。

## 示例

```cpp
template<typename T>
class SmartPtr {
public:
    template<typename U>
    SmartPtr(const SmartPtr<U> &other);  // 接受兼容类型
};
```

---

## 代码自测

**题目 1：** 智能指针类如何用成员函数模板接受所有兼容类型？
```cpp
template<typename T>
class SmartPtr {
    T* ptr;
public:
    // 如何让 SmartPtr<Derived> 能转为 SmartPtr<Base>？
};
```

<details>
<summary>参考答案</summary>

用泛化拷贝构造（成员函数模板）：
```cpp
template<typename T>
class SmartPtr {
public:
    template<typename U>
    SmartPtr(const SmartPtr<U>& other) : ptr(other.get()) {}
    T* get() const { return ptr; }
};
```
但需防止不兼容类型的转换（如 `SmartPtr<int>` 转 `SmartPtr<double>`），可加 `static_assert` 或 `enable_if`。同时还要声明普通的拷贝构造函数——否则编译器会生成默认的，遮蔽模板版本。

</details>
