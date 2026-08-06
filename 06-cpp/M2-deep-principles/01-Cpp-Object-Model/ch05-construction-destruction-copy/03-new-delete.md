# 5.3 new / delete 的两步

> 第 5 章 · 上一节：[5.2 存储期与生命周期](02-storage-duration.md) · 下一节：[5.4 异常安全](04-exception-safety.md)

## 这节讲什么

`new Widget` 不是一步——它先 `operator new` 分配内存，再调构造函数。`placement new` 在已分配内存上构造，省掉分配步骤。

---

## 两步机制

```cpp
Widget* p = new Widget;
// 展开：
// 1. operator new(sizeof(Widget)) → 分配内存
// 2. Widget::Widget() → 在该内存上构造

delete p;
// 展开：
// 1. p->~Widget() → 析构
// 2. operator delete(p) → 释放内存
```

### Placement New

```cpp
char buf[sizeof(Widget)];
Widget* p = new (buf) Widget;  // 在 buf 上构造，不分配
p->~Widget();                   // 手动析构（不 delete！）
```

`placement new` 在已分配内存上构造——不分配内存。HFT 用它配合 mempool 实现零 `malloc`。

---

## 新手要点（和 C 的区别）

- **C 用 malloc/free**：C 的 `malloc` 只分配内存，不调构造函数（C 没有构造函数）。C++ 的 `new` = 分配 + 构造，`delete` = 析构 + 释放。
- **别混用**：`malloc` + `delete` 或 `new` + `free` 是 UB——`malloc` 不会调构造，`free` 不会调析构。

---

## HFT 关联

- **placement new + mempool**：`new(membuf) Widget` 在预分配 mempool 上构造，零 `malloc`——HFT 对象池惯用法。
- **`operator new` 重载**：全局/类级重载 `operator new` 接 mempool/hugepage。

---

## 自测题

1. `new Widget` 的两步是什么？
2. `placement new` 省掉了哪一步？它为什么用于 mempool？
3. 为什么不能 `malloc` + `delete` 或 `new` + `free`？
4. HFT 如何用 `placement new` + mempool 实现零 `malloc`？

---

## 参考与延伸

- 下一节：[5.4 异常安全](04-exception-safety.md)
- 回到：[第 5 章](README.md)
