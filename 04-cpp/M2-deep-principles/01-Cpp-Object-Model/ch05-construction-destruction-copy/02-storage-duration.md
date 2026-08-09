# 5.2 存储期与生命周期

> 第 5 章 · 上一节：[5.1 构造与析构顺序](01-ctor-dtor-order.md) · 下一节：[5.3 new/delete 的两步](03-new-delete.md)

## 这节讲什么

局部、全局、堆对象的生命周期差异。全局对象的构造在 `main` 前——跨翻译单元的构造顺序未指定（static init order fiasco）。

---

## 三种存储期

| 存储期 | 构造时机 | 析构时机 |
|--------|----------|----------|
| 局部（栈） | 到达声明点 | 离开作用域 |
| 全局/静态 | `main` 前 | `main` 后（`atexit` 顺序） |
| 堆（`new`） | `new` 时 | `delete` 时（不 delete 则泄漏） |

### Static Init Order Fiasco

```cpp
// file1.cpp
extern int g_count;
int g_value = g_count + 1;  // g_count 可能还没构造！

// file2.cpp
int g_count = 42;
```

跨翻译单元的全局对象构造顺序未指定——`g_value` 可能在 `g_count` 之前构造，读到未初始化值。

**解法**：Meyers singleton（函数内 static）：
```cpp
int& count() { static int c = 42; return c; }
// 首次调用时构造，C++11 起线程安全
```

---

## 新手要点（和 C 的区别）

- **C 的全局变量也是 `main` 前初始化**：但 C 的初始化是静态的（编译期赋值），没有"构造顺序"问题。C++ 的全局对象有构造函数，运行时调用——才有顺序问题。
- **避免全局对象**：用 Meyers singleton（函数内 static）替代全局对象，避免 init order fiasco。

---

## HFT 关联

- **Meyers singleton**：HFT 守护进程的配置/单例用函数内 static，C++11 起保证线程安全初始化。

---

## 自测题

1. 全局对象的构造/析构在 `main` 的哪一侧？
2. 什么是 static init order fiasco？如何规避？
3. Meyers singleton 为什么能解决构造顺序问题？

---

## 参考与延伸

- 下一节：[5.3 new/delete 的两步](03-new-delete.md)
- 回到：[第 5 章](README.md)
