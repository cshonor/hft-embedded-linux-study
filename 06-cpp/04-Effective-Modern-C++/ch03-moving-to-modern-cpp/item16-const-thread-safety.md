# Item 16：让 const 成员函数线程安全

> 第 3 章 移步现代 C++ · Item 16 · 上一节：[Item 15 constexpr](item15-constexpr.md)

## 这节讲什么

`const` 成员函数仍可修改 `mutable` 成员（如缓存、互斥锁）。如果 `const` 函数会读写 `mutable` 成员，它就不是天然线程安全的——必须加锁或用 `std::atomic`。

---

## 核心问题

```cpp
class Cache {
    mutable int cachedValue;
public:
    int getValue() const {       // const 函数
        if (!cachedValue) cachedValue = compute();  // 修改 mutable 成员！
        return cachedValue;
    }
};
// 两个线程同时调 getValue() → 数据竞争（cachedValue 的读-改-写不是原子的）
```

修复：用 `std::atomic` 或 `mutex`：
```cpp
mutable std::atomic<int> cachedValue;  // 原子操作，线程安全
```

---

## 新手要点（和 C 的区别）

- **C 没有 const 成员函数**：C 的 `const` 只修饰变量/指针。C++ 的 `const` 成员函数承诺"不修改对象逻辑状态"，但 `mutable` 开了个后门用于缓存/锁。
- **陷阱**：看到 `const` 别以为线程安全——`mutable` 成员可能被改。多线程调 `const` 函数前确认它没有可变的 `mutable` 成员，或那些成员是原子的。

---

## HFT 关联

- **行情缓存**：`const` 的 `get_tick()` 如果内部更新 `mutable` 缓存，必须用 `atomic` 或 `mutex` 保护——否则多策略线程并发读会 data race。

---

## 自测题

1. `const` 成员函数为什么可能不是线程安全的？`mutable` 在其中扮演什么角色？
2. 修复 `const` 函数的数据竞争有哪两种方式？
3. 为什么 `mutex` 也要声明为 `mutable`？

---

## 参考与延伸

- 下一节：[Item 17 特殊成员函数生成规则](item17-special-member-functions.md)
- 回到：[第 3 章 移步现代 C++](README.md)
