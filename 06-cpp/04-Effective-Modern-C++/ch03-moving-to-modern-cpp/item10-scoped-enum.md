# Item 10：优先限定作用域枚举（enum class）

> 第 3 章 移步现代 C++ · Item 10 · 上一节：[Item 9 using](item09-using.md)

## 这节讲什么

`enum class`（限定作用域枚举）比传统 `enum` 更安全：不污染命名空间、不隐式转整型、可前向声明。

---

## 对比

```cpp
enum Color { Red, Green };          // unscoped：Red 泄漏到外层作用域
enum class Color2 { Red, Green };   // scoped：必须 Color2::Red
```

| 特性 | `enum` | `enum class` |
|------|--------|--------------|
| 命名空间 | 枚举值泄漏到外层 | 必须 `Color2::Red` |
| 隐式转整型 | 允许（`int x = Red;`） | **禁止**（必须 `static_cast`） |
| 前向声明 | 不能（除非指定底层类型） | 可以（默认 `int`） |

---

## 新手要点（和 C 的区别）

- **C 只有 unscoped enum**：C 的 `enum` 枚举值会泄漏到外层作用域，且能隐式转 `int`。C++ 的 `enum class` 修正了这两个问题。
- **一律用 enum class**：新代码全用 `enum class`，除非需要和 C 互操作（C 不认识 `enum class`）。
- **前向声明**：`enum class OrderSide : uint8_t;` 可以先声明后定义，减少编译依赖。

---

## HFT 关联

- **订单状态防误比较**：`enum class Side { Buy, Sell };` 编译器拒绝 `Side::Buy == 1` 的隐式比较，消除一类下单逻辑 bug。
- **协议字段**：FIX 字段标签用 `enum class Tag : uint32_t`，既类型安全又可指定底层类型节省内存。

---

## 自测题

1. `enum` 和 `enum class` 的三个主要区别是什么？
2. 为什么 `enum class` 能防止"订单状态和整数误比较"？
3. `enum class Color : uint8_t { ... }` 指定底层类型有什么好处？
4. 什么场景下仍需要用传统 `enum`？

---

## 参考与延伸

- 下一节：[Item 11 =default](item11-default.md)
- 回到：[第 3 章 移步现代 C++](README.md)
