# 2.4 NRVO（命名返回值优化）

> 第 2 章 · 上一节：[2.3 成员初始化列表](03-init-list.md) · 下一章：[第 3 章 数据语义](../ch03-data-semantics/README.md)

## 这节讲什么

编译器消除返回值的拷贝——`Widget make() { Widget w; return w; }` 的 `w` 直接在调用者栈上构造，零拷贝。C++17 起对返回纯右值是强制拷贝省略。

---

## 核心机制

```cpp
Widget make() {
    Widget w;
    // ... 操作 w ...
    return w;  // NRVO：w 直接在调用者栈构造，零拷贝
}
Widget result = make();  // 没有拷贝！
```

### C++17 强制拷贝省略

```cpp
Widget make() { return Widget(); }  // C++17 强制省略，保证零拷贝
```

C++17 前这是"允许优化"（编译器可选），C++17 起是"强制省略"（标准保证）。

---

## 新手要点

- **不要 `std::move` 返回局部变量**：`return std::move(w);` 会**阻止** NRVO——因为 `move` 把 `w` 变成右值引用，编译器不再能把它当命名变量优化。直接 `return w;` 让编译器做 NRVO。
- **RVO vs NRVO**：RVO（返回纯右值 `return Widget();`）C++17 强制省略；NRVO（返回命名变量 `return w;`）仍是由编译器可选的优化，但主流编译器都做。

---

## HFT 关联

- **工厂函数依赖 NRVO**：返回大对象的工厂函数靠 NRVO 消除拷贝。不要 `std::move` 返回局部变量——阻碍 NRVO。

---

## 自测题

1. NRVO 如何消除返回值拷贝？
2. 为什么不要对局部变量 `std::move` 返回？
3. C++17 的强制拷贝省逸和 NRVO 有什么区别？

---

## 参考与延伸

- 下一章：[第 3 章 数据语义](../ch03-data-semantics/README.md)
- 回到：[第 2 章 构造函数语义](README.md)
