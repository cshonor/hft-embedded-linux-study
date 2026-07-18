# 条款 47：用 traits 萃取类获取类型信息

## 本节讲什么

编译期拿到迭代器、对象类别，配合重载分发不同逻辑，STL 迭代器分类核心原理。

## 示例

```cpp
template<typename Iter>
struct iterator_traits;  // traits 萃取迭代器类型信息
// std::iterator_traits<Iter>::value_type
```
