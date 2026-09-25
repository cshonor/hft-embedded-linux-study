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

<details>
<summary>参考答案</summary>

1. 三个主要区别：①**作用域**：`enum class` 的枚举量被限定在枚举类型作用域内，必须写 `Side::Buy`，不会泄漏到外层作用域；传统 `enum` 的枚举量直接注入外围作用域（`Buy`）。②**类型安全**：`enum class` 的枚举量不会隐式转换成整型，也不能与整数直接比较/算术；传统 `enum` 会隐式转成整型。③**可指定底层类型**：`enum class` 可以显式指定 underlying type（`enum class E : uint8_t`），传统 enum（C++11 前）由实现决定，C++11 起的 unscoped enum 虽然也能写 `enum E : uint8_t`，但 scoped enum 是这一特性的主要用法。

2. 因为 `enum class` 的枚举量类型就是该枚举类型本身，标准不提供到整型的隐式转换，也没有与整数类型的内建比较运算符。所以 `Side::Buy == 1` 或 `OrderState::Filled == 3` 这类比较在编译期就被拒绝；要做数值比较必须显式 `static_cast`（如 `static_cast<int>(s)`），这一步显式转换会提醒程序员检查语义是否正确。

3. 好处有两点：①**控制存储大小**：可以把状态压缩到 1 字节（`uint8_t`），在协议字段、大规模订单簿状态数组里节省内存、提高 cache 命中率；②**前向声明与 ABI 稳定**：确定了底层类型后，枚举可以前向声明（`enum class E : uint8_t;`），改动枚举量不必重编译所有包含该头文件的代码，二进制布局也稳定可预测。

4. 主要场景：需要枚举量隐式转整型参与数值运算或位标志组合时——例如用 unscoped enum 定义位掩码并直接 `A | B`、`flags & A`，或作为数组下标/整型常量使用而不想到处 `static_cast`；还有需要把它当作"整型常量"喂给只接受 `int` 的旧接口（如某些 C API）时。另外，希望在作用域内直接使用短名字（`Red` 而非 `Color::Red`）也是常见理由。

5. 不能编译。`int x = s;` 失败：scoped enum 不能隐式转换成 `int`，必须写 `int x = static_cast<int>(s);`。`if (s == 0)` 也失败：`Side` 与 `int` 之间没有可用的内建比较运算符。修正后：
```cpp
auto v = static_cast<std::underlying_type_t<Side>>(s);
if (s == static_cast<Side>(0)) { }
```

</details>

---

## 参考与延伸

- 下一节：[Item 11 =default](item11-default.md)
- 回到：[第 3 章 移步现代 C++](README.md)
