# Item 26：避免对万能引用重载

> 第 5 章 · Item 26 · 上一节：[Item 25 move vs forward 使用](item25-move-vs-forward-usage.md)

## 这节讲什么

万能引用重载会"贪婪匹配"——几乎任何实参都最优匹配万能引用版本，导致其他重载被遮蔽。这是 C++ 重载决议最隐蔽的坑。

---

## 核心问题

```cpp
class Person {
public:
    template<class T> Person(T&& n);   // 贪婪：连 Person 本身、int 都匹配
    Person(int idx);                   // 被遮蔽
};
```

万能引用构造会"劫持"拷贝构造：
```cpp
Person p1("Alice");
Person p2(p1);    // 匹配万能引用版本而非拷贝构造！
// 因为 T&& 匹配 Person& 比 const Person& 更优
```

---

## 新手要点（和 C 的区别）

- **C 没有重载决议的这种复杂性**：C 没有模板和万能引用。C++ 的万能引用"太贪婪"是模板元编程的副作用。
- **规则**：不要对万能引用重载。如果需要泛型构造，用 Item 27 的替代方案。

---

## HFT 关联

- **策略工厂**：`template<class T> Strategy(T&& config)` 如果 `config` 是 `Strategy` 本身，会被万能引用劫持而非拷贝构造。

---

## 自测题

1. 为什么万能引用重载会"贪婪匹配"？
2. `Person p2(p1)` 在有万能引用构造时匹配哪个构造函数？为什么？
3. 如何避免万能引用重载的问题？

---

## 参考与延伸

- 下一节：[Item 27 替代方案](item27-alternatives.md)
- 回到：[第 5 章](README.md)
