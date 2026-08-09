# 条款 47：用 traits 萃取类获取类型信息

## 本节讲什么

编译期拿到迭代器、对象类别，配合重载分发不同逻辑，STL 迭代器分类核心原理。

## 示例

```cpp
template<typename Iter>
struct iterator_traits;  // traits 萃取迭代器类型信息
// std::iterator_traits<Iter>::value_type
```

---

## 代码自测

**题目 1：** 如何用 traits 类在编译期判断迭代器类型？
```cpp
template<typename IterT>
void advance(IterT& it, int n) {
    // 如何根据迭代器类型选择不同实现？
}
```

<details>
<summary>参考答案</summary>

用 `iterator_traits` + tag dispatch：
```cpp
template<typename IterT>
void advance(IterT& it, int n) {
    doAdvance(it, n,
        typename std::iterator_traits<IterT>::iterator_category());
}
template<typename IterT>
void doAdvance(IterT& it, int n, std::random_access_iterator_tag) {
    it += n;  // 随机访问迭代器直接 +=
}
template<typename IterT>
void doAdvance(IterT& it, int n, std::input_iterator_tag) {
    while (n--) ++it;  // 只能逐个 ++
}
```
Traits 类统一了迭代器类型信息的访问方式，tag dispatch 在编译期选择实现，零运行时开销。

</details>
