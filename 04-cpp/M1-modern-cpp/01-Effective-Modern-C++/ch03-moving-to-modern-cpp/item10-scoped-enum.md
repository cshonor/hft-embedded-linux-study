# Item 10：优先限定作用域枚举（enum class）

> 第 3 章 移步现代 C++ · Item 10 · 上一节：[Item 9 using](item09-using.md)

## 为什么要学这个（先建立直觉）

C 的 `enum` 用起来很顺手，但它在 C++ 里有三个陈年旧伤：

```c
enum Color { Red, Green, Blue };
enum Direction { Up, Down, Left, Right };

int x = Red;            // 枚举名直接泄进外层作用域
if (Red == Up) {}       // 编译通过！两个不同枚举的值都悄悄转成 int 再比较
```

第一，**命名污染**：枚举名 `Red`、`Up` 全部注入外围作用域，再写一个 `enum Traffic { Red, Yellow };` 就是重定义错误——大项目里你不得不用 `COLOR_RED`、`DIR_UP` 这种前缀土法隔离。第二，**隐式转整型**：枚举和 `int` 之间没有任何防火墙，`Red == Up`（都是 0）这种语义荒谬的比较编译器一声不吭。第三，**不能前向声明**：底层类型由编译器在看完所有枚举值之后才定，头文件里没法只写个声明。

C++11 的 `enum class`（限域枚举）把三个伤一次治好：枚举名关进枚举自己的作用域、禁止隐式转整型、底层类型默认 `int` 所以随时可前向声明。代价只是多敲 `Color::` 四个字符。

---

## 这节讲什么

1. `enum` vs `enum class` 的逐条对照（命名泄漏、隐式转换、前向声明、底层类型）；
2. `std::underlying_type_t` 取底层类型的正确姿势；
3. 前向声明规则：scoped 直接声明，unscoped 必须指定底层类型（附实测报错）；
4. 全部论断本机实测：clang 23.1.0，arm64-apple-darwin，libc++，`-std=c++17 -Wall`。

---

## enum vs enum class 总览对比

| 特性 | `enum`（unscoped） | `enum class`（scoped） |
|------|--------------------|------------------------|
| 枚举名作用域 | 泄漏到外层作用域 | 限定在枚举内，必须 `Color::Red` |
| 隐式转整型 | 允许 | **禁止**，必须 `static_cast` |
| 不同枚举间比较 | 允许（都转成 int） | **禁止**（编译错误） |
| 默认底层类型 | 编译器自选（不大于 int 且装得下） | `int`（固定） |
| 显式指定底层类型 | C++11 起可以 | 可以 |
| 前向声明 | 必须指定底层类型 | 直接可以（默认 `int` 已知） |

---

## 命名泄漏：实测重定义

```cpp
enum Color { Red, Green, Blue };
enum Traffic { Red, Yellow };   // 同作用域

int main() { return Red; }
```

本机实测 clang 23.1.0 报错原文：

```
error: redefinition of enumerator 'Red'
    2 | enum Traffic { Red, Yellow };
      |                ^
note: previous definition is here
    1 | enum Color { Red, Green, Blue };
      |              ^
```

unscoped 枚举的枚举名住在**外层**作用域，两个枚举撞名就是硬错误。换成 `enum class`，`Color::Red` 和 `Traffic::Red` 各住各家，互不干扰——连跨类型比较都被拦下：

```cpp
enum class Color { Red, Green, Blue };
enum class Traffic { Red, Yellow };

auto a = Color::Red;
auto b = Traffic::Red;
return a == b;   // 报错
```

实测报错：

```
error: invalid operands to binary expression ('Color' and 'Traffic')
note: no implicit conversion for scoped enum; consider casting to underlying type
```

unscoped 版本里这个比较会静默通过（都转成 `int`）——`Red == Up` 这种笑话就是从这里来的。

---

## 隐式转换：有和没有防火墙

```cpp
enum class Side { Buy, Sell };

int main() {
    Side s = Side::Buy;
    int x = s;            // 编译失败
    return x;
}
```

本机实测 clang 23.1.0 报错原文：

```
error: cannot initialize a variable of type 'int' with an lvalue of type 'Side'
    int x = s;
        ^   ~
```

要转换就显式写出来：

```cpp
int x = static_cast<int>(s);              // OK，x = 0
Side s2 = static_cast<Side>(1);           // 反向也要显式
if (s == Side::Buy) {}                    // 同类比较 OK
// if (s == 0) {}                         // 编译失败：无内建运算符
```

这道防火墙的工程价值：`Side::Buy == 1` 这种把语义比较写成数值比较的 bug，在编译期就被消灭。下单方向的枚举值和协议里的 `int` 字段永远不会被意外混用。

---

## 底层类型与前向声明

scoped 枚举默认底层类型是 `int`，且可随时前向声明；unscoped 枚举只有**显式指定**底层类型后才能前向声明：

```cpp
#include <cstdint>

enum class Side;                 // OK：scoped 默认 int，类型完整
enum Legacy : std::uint8_t;      // OK：unscoped 但指定了底层类型
// enum Old;                     // 报错！unscoped 未指定底层类型

void useSide(Side s);            // 只凭声明就能用
void useLegacy(Legacy l);
```

`enum Old;` 的实测报错原文：

```
error: ISO C++ forbids forward references to 'enum' types
    2 | enum Old;
      |      ^
```

前向声明的意义不只是少 include 一个头文件：底层类型固定后，枚举的**大小和 ABI 就锁死了**——枚举值增删不需要重编译所有使用者，二进制接口也不会漂移。

`sizeof` 实测（clang 23.1.0）：

```cpp
enum Color { Red, Green, Blue };              // unscoped
enum class Side { Buy, Sell };                // scoped，默认 int
enum class Status : unsigned char {           // 显式 1 字节
    Pending = 0, Active = 1, Closed = 255
};
// sizeof(Color) == 4   sizeof(Side) == 4   sizeof(Status) == 1
```

---

## std::underlying_type_t：和底层类型打交道

scoped 枚举不能隐式转整型，代码里就少不了"取底层类型"的操作。手写 `static_cast<int>` 会把类型写死，换成萃取：

```cpp
#include <type_traits>

auto raw = static_cast<std::underlying_type_t<Status>>(Status::Closed);
// raw 的类型自动是 unsigned char，改底层类型不用改调用点
```

本机实测断言通过：

```cpp
static_assert(std::is_same_v<std::underlying_type_t<Side>, int>);
static_assert(std::is_same_v<std::underlying_type_t<Status>, unsigned char>);
```

（`underlying_type_t` 是 C++14 的 `_t` 别名，C++11 要写 `typename std::underlying_type<T>::type`——正是 [Item 9](item09-using.md) 讲的那套。）

---

## 完整可运行例子

```cpp
#include <cstdint>
#include <iostream>
#include <type_traits>

enum Color { Red, Green, Blue };          // unscoped
enum class Side { Buy, Sell };            // scoped，默认 int
enum class Status : unsigned char { Pending = 0, Active = 1, Closed = 255 };

int main() {
    int r = Red;                            // unscoped 隐式转 int
    std::cout << "Red=" << r << "\n";

    Side s = Side::Buy;
    // int x = s;                           // error: 不能隐式转
    std::cout << "static_cast<int>(Side::Buy)=" << static_cast<int>(s) << "\n";

    std::cout << "sizeof(Color)=" << sizeof(Color) << "\n";
    std::cout << "sizeof(Side)=" << sizeof(Side) << "\n";
    std::cout << "sizeof(Status)=" << sizeof(Status) << "\n";

    static_assert(std::is_same_v<std::underlying_type_t<Side>, int>);
    static_assert(std::is_same_v<std::underlying_type_t<Status>, unsigned char>);

    auto raw = static_cast<std::underlying_type_t<Status>>(Status::Closed);
    std::cout << "Status::Closed raw=" << static_cast<int>(raw) << "\n";
}
```

本机实测 clang 23.1.0（`-std=c++17 -Wall`）输出：

```
Red=0
static_cast<int>(Side::Buy)=0
sizeof(Color)=4
sizeof(Side)=4
sizeof(Status)=1
Status::Closed raw=255
```

---

## 常见错误（新手踩坑）

1. **忘了写作用域限定**。`enum class Side { Buy, Sell }; Side s = Buy;` 编译失败——枚举名不外泄本来就是特性。修正：`Side::Buy`。
2. **拿枚举值和整数直接比较**。`if (s == 0)` 报 `invalid operands` 类错误。修正：`if (s == Side::Buy)`，语义上你也该和枚举值比而不是和魔法数字比。
3. **`static_cast<int>` 把底层类型写死**。枚举改成 `uint8_t` 后所有调用点 silently 语义漂移。修正：`static_cast<std::underlying_type_t<E>>(e)`。
4. **unscoped enum 想前向声明却没指定底层类型**。实测报 `ISO C++ forbids forward references to 'enum' types`。修正：`enum E : std::uint8_t;`，或者直接换 `enum class`。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 枚举声明 | `enum Color { Red, Green };` | `enum class Color { Red, Green };` | 防命名污染 |
| 访问枚举值 | `Red` | `Color::Red` | 限定作用域 |
| 和整数互转 | 隐式 | 必须 `static_cast` | 类型安全 |
| 前向声明 | 不行 | scoped 直接声明 | 减少编译依赖、锁死 ABI |
| 底层类型 | 编译器决定 | 可显式指定（`: uint8_t`） | 控制内存布局 |

**一句话总结：** C 程序员记住——新代码全用 `enum class`，除非要和 C API 互操作或者写位标志（`A | B` 这类运算 unscoped 更方便）。多打的 `Color::` 前缀换来的是编译期类型安全。

---

## HFT 关联

- **订单状态机**：`enum class OrderState : uint8_t { Pending, PartiallyFilled, Filled, Cancelled };`——1 字节存储、不能和整数误比较、`switch` 配 `-Wall` 漏分支会警告。三个特性全是白捡的。
- **协议字段锁死大小**：FIX/ITCH 报文里的标志字段用 `enum class Tag : uint32_t`，`sizeof` 可预测，序列化时 `static_cast<std::underlying_type_t<Tag>>(t)` 直接写进缓冲区，布局即 ABI。
- **缓存友好**：百万级订单簿条目里，状态字段从 4 字节压到 1 字节（指定 `uint8_t`），配合 `alignas` 能把热数据多塞 30%+ 进缓存行——前提是底层类型显式可控，unscoped enum 给不了这个保证。
- **前向声明减编译依赖**：交易所接口头文件只声明 `enum class Side : uint8_t;`，枚举值增删不触发下游重编译——对几十个子模块的 HFT 代码库是实打实的编译时间节省。

---

## 自测题

**1. `enum` 和 `enum class` 的三个主要区别是什么？**
<details><summary>答案</summary>
① 作用域：`enum class` 的枚举名限定在枚举内（`Side::Buy`），unscoped 泄漏到外层作用域；② 类型安全：`enum class` 不隐式转整型、不能和整数比较，unscoped 会隐式转；③ 前向声明：`enum class` 默认底层类型 `int` 可直接前向声明，unscoped 必须显式指定底层类型才行。
</details>

**2. 为什么 `enum class` 能前向声明而 unscoped enum 不能（不指定底层类型时）？**
<details><summary>答案</summary>
前向声明要求编译器在看不到枚举值列表时就知道类型的大小。`enum class` 底层类型默认 `int`，大小已知；unscoped enum 的底层类型要看完所有枚举值才能确定（取能装下的最小类型、不大于 int），大小未知所以不能声明。实测 `enum Old;` 报 `error: ISO C++ forbids forward references to 'enum' types`。
</details>

**3. `enum class Side { Buy, Sell }; Side s = Side::Buy; int x = s;` 能编译吗？**
<details><summary>答案</summary>
不能。实测报 `error: cannot initialize a variable of type 'int' with an lvalue of type 'Side'`。scoped 枚举到整型没有隐式转换，必须 `static_cast`——这就是它防止"枚举和整数误比较"类 bug 的机制。
</details>

**4. `enum class Status : uint8_t` 显式指定底层类型有什么好处？**
<details><summary>答案</summary>
① 控制大小：`sizeof(Status)==1`，协议字段和大规模数组省内存、提高缓存命中率；② ABI 稳定：布局锁死，枚举值增删不影响二进制接口；③ 可前向声明：减少编译依赖。unscoped enum 不指定时大小由编译器选，给不了这些保证。
</details>

**5. 怎么把一个 scoped 枚举值转成它的底层类型值，且不写死类型名？**
<details><summary>答案</summary>
```cpp
auto raw = static_cast<std::underlying_type_t<Side>>(s);
```
`underlying_type_t`（C++14；C++11 写 `typename std::underlying_type<T>::type`）在编译期取出枚举的底层类型，枚举改底层类型时调用点零改动。
</details>

**6. 什么场景下仍应该用传统 unscoped enum？**
<details><summary>答案</summary>
需要隐式转整型的场合：位标志组合（`flags & A`、`A | B` 不想到处 cast）、作为数组下标、喂给只接受 `int` 的 C API；以及刻意想让枚举名注入外层作用域做"命名的常量组"（如放进 struct 里模拟 scoped 效果的老技巧）。
</details>

---

## 参考与延伸

- 上一节：[Item 9 using 别名](item09-using.md)（`underlying_type_t` 正是 `_t` 别名模板）
- 下一节：[Item 11 =delete 优于 private 未定义](item11-default.md)
- 回到：[第 3 章 移步现代 C++](README.md)
