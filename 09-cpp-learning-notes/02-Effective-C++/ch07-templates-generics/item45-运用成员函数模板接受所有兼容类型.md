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
