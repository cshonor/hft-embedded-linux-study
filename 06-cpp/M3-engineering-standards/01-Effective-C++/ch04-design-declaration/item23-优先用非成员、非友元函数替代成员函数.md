# 条款 23：优先用非成员、非友元函数替代成员函数

## 本节讲什么

扩大函数扩展空间，不破坏类封装，STL 算法大多是非成员函数。

## 示例

```cpp
class UDate {
    friend bool checkValidity(const UDate &);
public:
    int month() const;
};
bool checkValidity(const UDate &d) { return d.month() <= 12; }
```
