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

<details>
<summary>参考答案</summary>

1. 标签分发把"万能引用的贪婪匹配"延后到重载决议已经确定的地方：外层 `log(T&& msg)` 仍然接受一切实参（保持泛型入口），但它不直接做实际工作，而是把 `std::forward<T>(msg)` 连同**一个编译期标签**（如 `std::is_integral<T>{}` / `std::true_type` / `std::false_type`）传给内部的 `log_impl`。标签是不同类型的空对象，因此 `log_impl` 的多个重载之间由**重载决议**在编译期选出，而不再依赖"万能引用更贪婪"的规则。这样既避免了万能引用重载被意外抢走调用（Item 26 的问题），也让分发逻辑显式可读、可静态断言。

2. `std::enable_if` 利用 SFINAE：当条件为假时让该模板的某个（默认）模板实参变成非法类型，从而把它从候选集中**静默移除**——于是"万能引用构造函数只在 T 不是 Person 时存在"，不会抢占拷贝构造。它本质是把约束编码进模板签名。C++20 用 **Concepts / `requires` 子句**替代：`template<class T> requires !std::is_same_v<T, Person>`——语义相同但直接表达"约束"，错误信息从一长串模板实例化噪声变成一句"约束未满足"，且可以命名复用（`template<std::integral T>`）。

3. 最推荐**方案 3（放弃万能引用重载，用具名重载）**。原因：它完全不依赖模板推导与 SFINAE 的微妙规则，行为直观、调用方看到的就是 `set(const std::string&)` / `set(int)`，不会有"传 `Person` 却匹配到万能引用"、"传 `long` 匹配错重载"这类意外；编译错误信息简单，调试成本低。代价是要多写几个重载、可能有少量拷贝，但在绝大多数真实接口里这点成本远小于因万能引用误匹配带来的隐蔽 bug。等熟悉模板后再用标签分发或 Concepts 做精细控制。

</details>

---

## 参考与延伸

- 下一节：[Item 28 引用折叠](item28-reference-collapsing.md)
- 回到：[第 5 章](README.md)
