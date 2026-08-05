# 第 5 章 构造、析构与拷贝

**Construction, Destruction, and Copy**

## 本章讲什么

对象从诞生到销毁的全过程——构造顺序、析构顺序、不同存储期（局部/全局/堆）的对象生命周期差异、`new`/`delete` 的底层行为。理解这些才能写出正确的资源管理与异常安全代码。

## 要点

### 构造与析构顺序

- **构造**：基类 → 成员对象（声明序） → 自身构造体。
- **析构**：自身析构体 → 成员对象（声明逆序） → 基类（逆序）。
- 全程严格对称——构造了什么，析构就按反序拆什么。

### 存储期与生命周期

| 存储期 | 构造时机 | 析构时机 |
|--------|----------|----------|
| 局部（栈） | 到达声明点 | 离开作用域 |
| 全局/静态 | `main` 前 | `main` 后（`atexit` 顺序） |
| 堆（`new`） | `new` 时 | `delete` 时（不 delete 则泄漏） |

全局对象的构造在 `main` 前（`__cxa_init`），析构在 `main` 后——跨翻译单元的全局对象构造顺序未指定（`static init order fiasco`）。

### `new` / `delete` 的两步

```cpp
Widget* p = new Widget;   // 1. operator new 分配内存  2. 构造函数构造
delete p;                  // 1. 析构函数析构  2. operator delete 释放
```
`operator new` 默认调 `malloc`，可重载接 mempool。`placement new` 在已分配内存上构造（不分配）。

### 异常安全

构造函数抛异常：已构造的成员/基类按逆序析构，但 `operator new` 分配的内存由运行时自动释放（RAII）。析构函数抛异常危险（析构栈展开期间抛异常 → `terminate`）——析构函数应 `noexcept`。

## HFT 关联

- **全局对象 init order fiasco**：HFT 守护进程的全局配置/单例跨翻译单元依赖会触发构造顺序问题——用 `Meyers singleton`（函数内 static）或显式 init 函数规避。
- **`placement new` + mempool**：`new(membuf) Widget` 在预分配 mempool 上构造，零 `malloc`——HFT 对象池惯用法。
- **析构 `noexcept`**：HFT 析构绝不抛异常（否则 `terminate` 拉崩进程）。

## 自测题

1. 构造和析构的顺序分别是什么？为什么严格对称？
2. 全局对象的构造/析构在 `main` 的哪一侧？跨翻译单元的构造顺序有什么问题？
3. `new Widget` 的两步是什么？`placement new` 省掉了哪一步？
4. 构造函数抛异常时内存会泄漏吗？析构函数抛异常为什么危险？
5. HFT 如何用 `placement new` + mempool 实现零 `malloc` 对象池？
