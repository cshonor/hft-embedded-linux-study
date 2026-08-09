# Item 10：优先限定作用域枚举（enum class）

> 第 3 章 移步现代 C++ · Item 10 · 上一节：[Item 9 using](item09-using.md)

## 为什么要学这个（先建立直觉）

C 程序员对 `enum` 很熟悉：

```c
enum Color { Red, Green, Blue };
enum Direction { Up, Down, Left, Right };

int x = Red;       // OK——Red 泄漏到外层作用域
int y = Up;        // OK——Up 也泄漏了
if (Red == Up) { } // 编译通过！Red=0, Up=0，两个不同枚举的值可以比较
```

C 的 `enum` 有三个问题：
1. **命名空间污染**——枚举值泄漏到外层作用域，`Red` 和 `Up` 可能冲突
2. **隐式转整型**——`int x = Red;` 不经意就转了，失去类型安全
3. **不能前向声明**——不指定底层类型时，编译器需要看到完整定义才能确定大小

C++11 的 `enum class` 修正了全部三个问题：

```cpp
enum class Color { Red, Green, Blue };
Color c = Color::Red;        // 必须 Color::Red
int x = Color::Red;          // 编译失败！不能隐式转整型
// if (Color::Red == Direction::Up) { }  // 编译失败！不同枚举类型不能比较
```

---

## 这节讲什么

`enum class`（限定作用域枚举）比传统 `enum` 更安全：不污染命名空间、不隐式转整型、可前向声明。

---

## 详细对比

```cpp
// 传统 enum（unscoped）
enum Color { Red, Green };          // Red 泄漏到外层作用域
enum TrafficLight { Red, Yellow };  // 编译失败！Red 重定义

// enum class（scoped）
enum class Color2 { Red, Green };   // Color2::Red，不泄漏
enum class TrafficLight2 { Red, Yellow };  // OK，TrafficLight2::Red 不冲突
```

| 特性 | `enum` | `enum class` |
|------|--------|--------------|
| 命名空间 | 枚举值泄漏到外层 | 必须 `Color2::Red` |
| 隐式转整型 | 允许（`int x = Red;`） | **禁止**（必须 `static_cast`） |
| 前向声明 | 不能（除非指定底层类型） | 可以（默认 `int`） |
| 底层类型 | 不指定时由编译器决定 | 可显式指定（`: uint8_t`） |
| 不同 enum 间比较 | 允许（都转成 int） | **禁止**（类型安全） |

### 指定底层类型

```cpp
// enum class 默认底层类型是 int
enum class Status : uint8_t {   // 指定为 uint8_t，节省内存
    Pending = 0,
    Active = 1,
    Closed = 255
};
// sizeof(Status) == 1 —— 适合网络协议字段

// 前向声明（只需指定底层类型，不需要看到完整定义）
enum class OrderSide : uint8_t;  // 前向声明
void process(OrderSide side);    // 可以用，不需要完整的 enum 定义
// 后面再定义
enum class OrderSide : uint8_t { Buy, Sell };
```

### 显式转换

```cpp
enum class Side { Buy, Sell };
Side s = Side::Buy;

// int x = s;              // 编译失败！不能隐式转
int x = static_cast<int>(s);  // OK，显式转换，x = 0

// 从整型转回来
Side s2 = static_cast<Side>(1);  // OK，s2 = Side::Sell
```

---

## 常见错误（新手踩坑）

**错误 1：忘了加作用域限定**
```cpp
enum class Side { Buy, Sell };
Side s = Buy;  // 编译失败！必须 Side::Buy
```
**修正：** `Side s = Side::Buy;`。这恰好是 `enum class` 的安全特性——防止命名冲突。

**错误 2：想直接和整数比较**
```cpp
enum class Status { Ok, Error };
Status s = Status::Ok;
if (s == 0) { }  // 编译失败！不能隐式转整型
```
**修正：** `if (s == Status::Ok)` 或 `if (static_cast<int>(s) == 0)`。

**错误 3：传统 enum 的命名冲突**
```cpp
enum Color { Red, Green, Blue };
enum Fruit { Apple, Orange, Red };  // 编译失败！Red 重定义
```
**修正：** 用 `enum class`，`Color::Red` 和 `Fruit::Red` 不会冲突。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 枚举声明 | `enum Color { Red, Green };` | `enum class Color { Red, Green };` | 防止命名空间污染 |
| 访问枚举值 | `Red` | `Color::Red` | 限定作用域，防冲突 |
| 和整数互转 | 隐式转换 | 必须 `static_cast` | 类型安全 |
| 前向声明 | 不行 | `enum class Color : uint8_t;` | 减少编译依赖 |
| 底层类型 | 编译器决定 | 可显式指定 | 控制内存布局 |

**一句话总结：** C 程序员记住——新代码全用 `enum class`，除非需要和 C 互操作。多打 `Color::` 前缀换来的是编译期类型安全。

---

## HFT 关联

- **订单状态防误比较**：`enum class Side { Buy, Sell };` 编译器拒绝 `Side::Buy == 1` 的隐式比较，消除一类下单逻辑 bug。
- **协议字段**：FIX 字段标签用 `enum class Tag : uint32_t`，既类型安全又可指定底层类型节省内存。
- **状态机**：`enum class OrderState : uint8_t { Pending, PartiallyFilled, Filled, Cancelled };` 用 1 字节存储状态，配合 `switch` 编译器会检查是否覆盖所有分支。

---

## 自测题

1. `enum` 和 `enum class` 的三个主要区别是什么？
2. 为什么 `enum class` 能防止"订单状态和整数误比较"？
3. `enum class Color : uint8_t { ... }` 指定底层类型有什么好处？
4. 什么场景下仍需要用传统 `enum`？
5. 下面代码能编译吗？
```cpp
enum class Side { Buy, Sell };
Side s = Side::Buy;
int x = s;
if (s == 0) { }
```

---

## 参考与延伸

- 下一节：[Item 11 =default](item11-default.md)
- 回到：[第 3 章 移步现代 C++](README.md)
