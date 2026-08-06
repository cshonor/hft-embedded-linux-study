# Item 9：优先 using 别名而非 typedef

> 第 3 章 移步现代 C++ · Item 9 · 上一节：[Item 8 nullptr](item08-nullptr.md)

## 这节讲什么

`using` 和 `typedef` 功能相同，但 `using` 支持模板化（alias template），`typedef` 不行。且 `using` 的语法更直观。

---

## 核心区别

```cpp
// 普通别名：两者等价
typedef unsigned long ulong;
using ulong = unsigned long;

// 模板别名：只有 using 可以
template<class T> using Vec = std::vector<T, MyAlloc<T>>;  // OK
template<class T> typedef std::vector<T, MyAlloc<T>> Vec;  // 编译失败！
```

`using` 还能避免读复杂的函数指针类型：
```cpp
typedef void (*Callback)(int, std::string);   // 难读
using Callback = void(*)(int, std::string);    // 清晰
```

---

## 新手要点（和 C 的区别）

- **C 只有 typedef**：C 程序员习惯 `typedef unsigned long ulong;`。C++ 里用 `using` 替代，语法更自然（`using 新名 = 原类型`，从左到右读）。
- **模板别名是 using 独有**：当需要在模板里定义类型别名时，`typedef` 需要 `typename` + 嵌套结构，`using` 直接搞定。
- **一律用 using**：新代码全用 `using`，`typedef` 只在维护老代码时遇到。

---

## HFT 关联

- **自定义分配器容器**：`template<class T> using PoolVec = std::vector<T, PoolAlloc<T>>;` 让 HFT 内存池容器有简洁的别名。

---

## 自测题

1. `using` 和 `typedef` 的功能区别是什么？
2. 为什么 `typedef` 不能定义模板别名而 `using` 可以？
3. `using Callback = void(*)(int);` 比 `typedef void (*Callback)(int);` 好在哪？

---

## 参考与延伸

- 下一节：[Item 10 scoped enum](item10-scoped-enum.md)
- 回到：[第 3 章 移步现代 C++](README.md)
