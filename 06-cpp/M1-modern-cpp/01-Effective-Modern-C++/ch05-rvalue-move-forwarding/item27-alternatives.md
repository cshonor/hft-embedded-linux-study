# Item 27：万能引用重载的替代方案

> 第 5 章 · Item 27 · 上一节：[Item 26 避免万能引用重载](item26-avoid-overloading-universal.md)

## 这节讲什么

当你确实需要泛型构造但又想避免万能引用重载的问题时，有三种替代方案。

---

## 三种替代方案

### 1. 标签分发（tag dispatch）

```cpp
template<class T>
void log(T&& msg) {
    log_impl(std::forward<T>(msg), std::is_integral<T>{});
    // 按是否整型分派到不同重载，避免万能引用贪婪匹配
}
```

### 2. enable_if 约束模板

```cpp
template<class T,
         class = std::enable_if_t<!std::is_same_v<T, Person>>>
Person(T&& n) : name(std::forward<T>(n)) {}
// 仅当 T 不是 Person 本身才启用万能引用构造
```

C++20 用 Concepts 更干净：
```cpp
template<class T> requires !std::is_same_v<T, Person>
Person(T&& n) : name(std::forward<T>(n)) {}
```

### 3. 放弃万能引用重载

直接用具名参数重载：
```cpp
void set(const std::string& s);
void set(int idx);
```

---

## 新手要点

- **新手用方案 3**：直接用具名重载，不碰万能引用。等熟悉模板后再学标签分发和 `enable_if`。
- **C++20 Concepts** 是 `enable_if` 的现代替代——语法更清晰，错误信息更友好。

---

## 自测题

1. 标签分发如何避免万能引用重载的问题？
2. `enable_if` 的作用是什么？C++20 用什么替代它？
3. 新手最推荐哪种方案？为什么？

---

## 参考与延伸

- 下一节：[Item 28 引用折叠](item28-reference-collapsing.md)
- 回到：[第 5 章](README.md)
