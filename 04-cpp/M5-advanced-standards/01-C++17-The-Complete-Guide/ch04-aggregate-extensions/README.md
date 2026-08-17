# 第 4 章 聚合类型扩展

**Aggregate Extensions**

## 本章讲什么

C++17 扩展了"聚合类型"（aggregate）的定义，让带基类的类也能用聚合初始化（花括号列表），并新增嵌套花括号省略规则。

## 要点

### 聚合类型的新定义

C++17 的聚合类型条件：
1. 没有**用户声明**的构造函数（`= default` 不算）。
2. 没有 private/protected 非静态数据成员。
3. 没有 virtual 函数。
4. 没有 virtual/private/protected 基类。
5. **C++17 新增**：可以有 public 基类（之前完全不能有基类）。

```cpp
// C++17：带基类的聚合
struct Base { int x; };
struct Derived : Base { int y; };

Derived d{ {1}, 2 };     // {1} 初始化 Base，2 初始化 y
Derived d2{ 1, 2 };       // 也可省略嵌套花括号，1 给 Base.x，2 给 y
```

### 嵌套花括号省略

C++17 进一步明确了花括号省略规则，让嵌套聚合/数组的初始化更灵活：

```cpp
struct Inner { int a, b; };
struct Outer { Inner i; int c; };

Outer o{ 1, 2, 3 };       // 等价于 { {1,2}, 3 }，省略内层 {}
Outer o2{ {1,2}, 3 };     // 显式写也行
```

### `std::is_aggregate`

```cpp
#include <type_traits>
static_assert(std::is_aggregate_v<Derived>);  // C++17
```

### 聚合的用途

- **POD 风格数据结构**：行情/订单字段结构体用聚合，零构造开销、可 memcpy。
- **继承的扁平数据**：基类放公共字段，派生类加特化字段，仍能用 `{}` 初始化。

## HFT 关联

- **tick/order 用聚合**：`struct Tick { int64_t ts; double px; int64_t qty; };` 是聚合，`Tick t{ts, px, qty}` 零开销、cache 友好。
- **继承的配置结构**：`struct BaseCfg { int timeout; }; struct FeedCfg : BaseCfg { string url; };` 仍能 `FeedCfg c{ {1000}, "tcp://..."}`。
- **`is_aggregate` 做静态断言**：模板里 `static_assert(is_aggregate_v<T>)` 确保传入类型是 POD 风格，可安全 memcpy/原子拷贝。
- **避免虚函数**：聚合不能有虚函数，天然引导用 CRTP/组合替代虚继承——HFT 偏好。

## 自测题

1. C++17 聚合类型的新定义相比 C++14 有什么变化？
2. 带基类的聚合如何用花括号初始化？嵌套花括号能省略吗？
3. 聚合类型不能有哪些东西？（4 条限制）
4. `std::is_aggregate` 有什么用？
5. HFT 为什么偏好把 tick/order 设计成聚合类型？

## 代码自测

### Q1: 聚合体扩展初始化
```cpp
struct Data {
    int id;
    std::string name;
    std::vector<int> values;
};

// C++14
Data d{1, "test", {1, 2, 3}};

// 带基类的聚合（C++17）
struct Base { int x; };
struct Derived : Base { int y; };
Derived d2{{1}, 2};  // C++17: 基类也用 {}
```
> C++17 放宽了聚合体的哪些限制？

<details>
<summary>答案与复习指引</summary>

C++17 聚合体扩展：
1. **基类**：有 public 基类的类也可以是聚合体，用嵌套 `{}` 初始化基类成员
2. **`explicit` 构造函数**：有 user-provided 构造函数不再阻止聚合（但标准有细化）
3. **去除 `explicit`** 的默认构造函数不影响聚合性

**注意**：`Derived d2{{1}, 2}` — 外层 `{}` 是 Derived，内层 `{1}` 初始化 Base 的 x，`2` 初始化 y。

**复习：** → [聚合体扩展](./README.md)
</details>
