# 条款 1：视 C++ 为一个语言联邦

## 本节讲什么

C++ 已从早期的 **「C with Classes」** 演化为**多范式**语言：过程化、面向对象、泛型、元编程并存。不要把它当成「一套规则走天下」的单一语言，而应视为由 **4 个子语言（sublanguages）** 组成的**联合体（federation of languages）**——从一种子语言切到另一种时，高效编码的惯用法也会变。

## 四种子语言

### 1. C 语言

C++ 的根基仍在 C：语句、预处理器、内建类型、数组、指针等。写这部分代码时，**没有**模板、异常、函数重载等机制，按 C 的思路来。

### 2. Object-Oriented C++（面向对象的 C++）

类（含构造/析构）、封装、继承、多态、虚函数（动态绑定）。适用经典 OOP 设计规则；自定义类型往往有构造/析构成本。

### 3. Template C++（模板 C++）

泛型编程；模板影响 C++ 的方方面面，并带来 **模板元编程（TMP）** 这一新范式。编译期往往**不知道**具体操作对象的完整类型。

### 4. STL（标准模板库）

容器、迭代器、算法、函数对象的高度整合。与 STL 协作时，要遵循其**独特规约**——迭代器、函数对象在设计上以 **C 指针**为原型。

## 为什么重要：传参方式因「子语言」而异

同一问题「参数怎么传最高效？」，在不同子语言里答案不同（详见 [条款 20](../ch04-design-declaration/item20-优先使用const引用传参，而非值传递.md)）：

| 子语言 | 典型场景 | 更优传参 |
|--------|----------|----------|
| **C** | `int`、`double` 等内建类型 | **传值** 通常更高效 |
| **OOP C++** | 自定义 `class`，有构造/析构 | **传 const 引用** |
| **Template C++** | 模板参数类型未知 | **传 const 引用**（仍安全且少拷贝） |
| **STL** | 迭代器、函数对象 | **传值** 又常回到 C 时代的老规则 |

```cpp
void f(int x);                          // C：内建类型，传值
void g(const Widget& w);                // OOP：自定义类型，const 引用
template<typename T> void h(const T& t); // Template：const 引用
void algo(std::vector<int>::iterator it); // STL：迭代器常按值/类似指针语义使用
```

**要点**：不是「C++ 永远传引用」或「永远传值」，而是**先判断当前代码落在哪个子语言**，再选惯用法。

## 与子语言对应的代码形态

```cpp
// 1. C 子集
char buf[] = "tick";
int price = 100;

// 2. OOP
class OrderBook {
public:
    void on_trade(double px) const { /* ... */ }
};

// 3. Template C++
template<typename PriceT>
PriceT mid(PriceT bid, PriceT ask) { return (bid + ask) / 2; }

// 4. STL
#include <vector>
#include <algorithm>
std::vector<int> levels{100, 101, 102};
std::sort(levels.begin(), levels.end());
```

## HFT 视角（读法提示）

低延迟系统里往往**分层混用**四种子语言，而不是只写一种风格：

- **热路径 / 协议字段**：C 子集思维——内建类型、数组、指针、位布局，少虚函数与异常；
- **业务对象 / 风控模块**：OOP——封装状态与接口；
- **泛型容器 / 编译期优化**：Template + TMP——`if constexpr`、策略类；
- **离线处理 / 工具链**：STL——`vector`、算法、迭代器遍历。

复习时问一句：**这段代码主要属于哪个子语言？** 再决定传参、错误处理、是否用虚函数等。

## 核心总结

1. C++ = **C + OOP + Template + STL** 四部分组成的联邦，不是单一规则表。
2. **切换子语言 → 切换惯用法**（传参、错误处理、抽象程度都会变）。
3. 传参速记：**内建类型 / STL 迭代器 → 常传值；自定义类型 / 模板参数 → 常 const 引用**（有例外，以 profiling 为准）。
4. 全书后续条款都建立在这个划分上——读每条时想它主要约束哪一「子语言」。
