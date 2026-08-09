# Item 26：避免对万能引用重载

> 第 5 章 · Item 26 · 上一节：[Item 25 move vs forward 使用](item25-move-vs-forward-usage.md)

## 为什么要学这个（先建立直觉）

C 没有函数重载和模板——函数名唯一对应一个函数。C++ 有重载和模板，万能引用 `T&&` 能匹配几乎任何类型，导致"贪婪匹配"问题。

```cpp
class Person {
public:
    template<class T> Person(T&& n);   // 万能引用构造函数
    Person(int idx);                   // int 构造函数
};

Person p1("Alice");     // 匹配万能引用？还是 int？→ 万能引用（更优匹配）
Person p2(42);          // 匹配万能引用？还是 int？→ 万能引用！（T = int&& 比 int 更优）
Person p3(p1);          // 匹配拷贝构造？还是万能引用？→ 万能引用！（T = Person& 比 const Person& 更优）
```

万能引用构造函数会"劫持"几乎所有其他构造函数，包括拷贝构造——这是 C++ 重载决议最隐蔽的坑。

---

## 这节讲什么

万能引用重载会"贪婪匹配"——几乎任何实参都最优匹配万能引用版本，导致其他重载被遮蔽。这是 C++ 重载决议最隐蔽的坑。

---

## 核心问题

### 万能引用劫持拷贝构造

```cpp
class Person {
    std::string name;
public:
    // 万能引用构造
    template<class T> Person(T&& n) : name(std::forward<T>(n)) {}

    // 拷贝构造（编译器生成的）
    // Person(const Person&) = default;
};

Person p1("Alice");
Person p2(p1);    // 你以为调拷贝构造？→ 匹配万能引用版本！
// 因为 T&& 推成 Person&，比 const Person& 更优匹配
// name(std::forward<Person&>(p1)) → name(p1) → 如果 name 类型不匹配 → 编译错误
```

### 万能匹配遮蔽其他重载

```cpp
class Person {
public:
    template<class T> Person(T&& n);   // 匹配一切
    Person(int idx);                   // 被遮蔽！
};

Person p(42);  // 匹配万能引用（T=int&&），不是 Person(int)
// 因为 T&& → int&& 比 int 更优（不需要转换）
```

### 派生类也有问题

```cpp
class Derived : public Person {
public:
    Derived(const Derived& d) : Person(d) {}  // 传 d 给 Person
    // Person(d) → 匹配万能引用（T=Derived&），不是拷贝构造！
    // 如果 Person 的万能引用构造不接受 Derived → 编译错误
};
```

---

## 常见错误（新手踩坑）

**错误 1：万能引用构造劫持拷贝构造**
```cpp
class Widget {
public:
    template<class T> Widget(T&& x) { /* ... */ }
};
Widget w1;
Widget w2(w1);  // 匹配万能引用，不是拷贝构造！可能编译错误
```
**修正：** 用 Item 27 的替代方案（SFINAE、tag dispatch、enable_if）。

**错误 2：万能引用重载遮蔽 int 构造**
```cpp
class Config {
public:
    template<class T> Config(T&& v);
    Config(int id);
};
Config c(42);  // 匹配万能引用（T=int&&），不是 Config(int)
```
**修正：** 用 SFINAE 约束万能引用（`enable_if`）或用 Item 27 的 tag dispatch。

**错误 3：派生类拷贝传给基类万能引用构造**
```cpp
class Base {
public:
    template<class T> Base(T&& x) { /* ... */ }
};
class Derived : public Base {
public:
    Derived(const Derived& d) : Base(d) {}  // Base(d) 匹配万能引用！
};
```
**修正：** 约束万能引用不接受派生类（`enable_if` + `is_base_of`）。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 函数重载 | 不支持 | 支持 | C++ 特性 |
| 泛型 | 宏/void* | 模板 + 万能引用 | 类型安全 |
| 重载冲突 | 不存在 | 万能引用贪婪匹配 | T&& 匹配范围太广 |
| 解决方案 | 不适用 | SFINAE/tag dispatch | Item 27 详谈 |

**一句话总结：** C 程序员记住——C++ 的万能引用 `T&&` 太贪婪，会"吃掉"其他重载。避免对万能引用重载，用 Item 27 的替代方案。

---

## HFT 关联

- **策略工厂**：`template<class T> Strategy(T&& config)` 如果 `config` 是 `Strategy` 本身，会被万能引用劫持而非拷贝构造。
- **配置传递**：泛型配置接收函数如果用万能引用重载，可能遮蔽其他构造函数。
- **编译错误难定位**：万能引用劫持导致的编译错误信息通常很长且不直观——预防胜于调试。

---

## 自测题

1. 为什么万能引用重载会"贪婪匹配"？
2. `Person p2(p1)` 在有万能引用构造时匹配哪个构造函数？为什么？
3. 如何避免万能引用重载的问题？
4. 下面代码有什么问题？
```cpp
class Buffer {
public:
    template<class T> Buffer(T&& data) : buf(std::forward<T>(data)) {}
    Buffer(int size);
};
Buffer b(1024);
```

---

## 参考与延伸

- 下一节：[Item 27 替代方案](item27-alternatives.md)
- 回到：[第 5 章](README.md)
