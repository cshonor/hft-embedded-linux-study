# 5.4 异常安全

> 第 5 章 · 上一节：[5.3 new/delete 的两步](03-new-delete.md) · 下一章：[第 6 章 运行时语义](../ch06-runtime-semantics/README.md)

## 这节讲什么

构造函数抛异常时内存不泄漏（RAII），但析构函数抛异常危险（`terminate`）。析构函数应 `noexcept`。

---

## 构造抛异常

```cpp
Widget* p = new Widget(args);
// 如果 Widget 构造抛异常：
// 1. 已构造的成员/基类按逆序析构
// 2. operator new 分配的内存自动释放
// → 内存不泄漏（RAII 保证）
```

### 析构抛异常 = 灾难

```cpp
~Widget() {
    if (cond) throw std::runtime_error("...");  // 危险！
}
// 析构在栈展开（异常传播）期间被调用
// 栈展开期间再抛异常 → std::terminate → 程序崩溃
```

**规则：析构函数绝不抛异常。** 标 `noexcept`，内部 catch 所有异常。

---

## 新手要点

- **C 没有异常**：C 程序员不习惯异常安全。C++ 的异常是运行时机制——构造失败抛异常，资源自动释放（RAII）。
- **析构 `noexcept`**：所有析构函数默认 `noexcept`（C++11 起）。别改成可能抛异常的——栈展开期间抛异常会 `terminate`。

---

## HFT 关联

- **析构 `noexcept`**：HFT 析构绝不抛异常（否则 `terminate` 拉崩进程）。
- **异常当致命错误**：HFT 把异常当"不可恢复错误"用（崩溃重启），不当控制流。

---

## 自测题

1. 构造函数抛异常时内存会泄漏吗？为什么？
2. 析构函数抛异常为什么危险？
3. 为什么析构函数应标 `noexcept`？
4. RAII 如何保证构造失败时资源不泄漏？

---

## 参考与延伸

- 下一章：[第 6 章 运行时语义](../ch06-runtime-semantics/README.md)
- 回到：[第 5 章](README.md)
