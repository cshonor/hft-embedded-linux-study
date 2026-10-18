# Item 22：用 Pimpl 惯用法降低编译依赖

> 第 4 章 智能指针 · Item 22 · 上一节：[Item 21 make 函数](item21-make-functions.md) · 下一章：[第 5 章 右值引用、移动语义与完美转发](../ch05-rvalue-move-forwarding/README.md)

## 为什么要学这个（先建立直觉）

C++ 的编译模型有个结构性痛点：头文件里类的**私有成员**也是类定义的一部分——你改一行私有实现（加个 `vector` 成员、换个第三方库），所有包含这个头文件的翻译单元全部重编译。大项目里改一个核心头文件触发几百个 TU 重编，"喝杯咖啡等编译"就是这么来的。

Pimpl（Pointer to Implementation）的思路釜底抽薪：头文件里只留一个指向实现结构的指针，实现整体挪进 `.cpp`。私有成员随便改，只有那一个 `.cpp` 重编译。代价是多一次间接和多一次堆分配——所以它是**接口类的工具**，不是热路径数据结构的工具。

---

## 这节讲什么

1. 三文件完整可编译结构（`widget.h` / `widget.cpp` / `main.cpp`）；
2. **编译防火墙收益的实测数字**：预处理行数、编译耗时、改实现后用户 TU 是否重编；
3. 核心规则：析构（和移动）为什么必须在 `.cpp` 定义，含报错原文；
4. `unique_ptr` vs `shared_ptr` 做 Pimpl 的区别、运行期成本实测（一次堆分配 + 一次间接寻址）、什么时候不该用 Pimpl。

---

## 核心结构（三文件，实测可编译）

```cpp
// ============ widget.h ============
#pragma once
#include <memory>

class Widget {
    struct Impl;                    // 只有前向声明：调用方看不见实现
    std::unique_ptr<Impl> pImpl;    // 不完整类型可以做 unique_ptr 成员
public:
    Widget();
    ~Widget();                              // ★ 只声明，定义必须在 .cpp
    Widget(Widget&&) noexcept;              // ★ 移动也要去 .cpp 定义
    Widget& operator=(Widget&&) noexcept;
    Widget(const Widget&) = delete;         // 拷贝默认禁用（拷贝需要 Impl 完整）
    Widget& operator=(const Widget&) = delete;
    int size() const;
    void add(int v);
    int sum() const;
};

// ============ widget.cpp ============
#include "widget.h"
#include <vector>

struct Widget::Impl {               // 实现细节全在这里
    std::vector<int> data;
};

Widget::Widget()  : pImpl(std::make_unique<Impl>()) {}
Widget::~Widget() = default;        // Impl 在此处已完整，default 安全
Widget::Widget(Widget&&) noexcept = default;
Widget& Widget::operator=(Widget&&) noexcept = default;
int  Widget::size() const { return static_cast<int>(pImpl->data.size()); }
void Widget::add(int v)   { pImpl->data.push_back(v); }
int  Widget::sum()  const { int s = 0; for (int v : pImpl->data) s += v; return s; }

// ============ main.cpp ============
#include "widget.h"
#include <iostream>
#include <utility>
int main() {
    Widget w;
    w.add(1); w.add(2); w.add(3);
    std::cout << "sizeof(Widget) = " << sizeof(Widget) << "\n";
    std::cout << "w.size() = " << w.size() << "  w.sum() = " << w.sum() << "\n";
    Widget w2 = std::move(w);       // 移动 OK（定义在 .cpp）
    std::cout << "after move: w2.size() = " << w2.size() << " w2.sum() = " << w2.sum() << "\n";
}
```

本机实测 clang 23.1.0（`-std=c++17 -Wall`）编译运行输出：

```
sizeof(Widget) = 8
w.size() = 3  w.sum() = 6
after move: w2.size() = 3 w2.sum() = 6
```

**调用方视角的关键数字：`sizeof(Widget) = 8`**。不管 `Impl` 里塞多少成员（vector、map、第三方库对象），`Widget` 永远是一个指针的大小——这正是 ABI 稳定的来源。

---

## 编译防火墙：收益实测

同一个"用户 TU"在两个 header 版本下的对比（非 Pimpl 版头文件带 `<vector> <map> <string> <deque>`，Pimpl 版只带 `<memory>`）实测：**预处理后行数 35798 → 13531（2.6x）**，`-fsyntax-only` 耗时 **0.270 s → 0.120 s（2.25x）**，改实现后用户 `.o` **不需要重编**。

最后一条才是 Pimpl 的真正价值。实测：给 `Widget::Impl` 加两个成员（`std::map` + `std::string`）、`widget.cpp` 加两个 `#include`，重编 `widget.o` 后**直接复用原来的 `main.o` 链接**，程序照常输出 `sizeof(Widget) = 8 / w.size() = 3`；而用户 `main.o` 的 md5 前后都是 `e73c115f2e5f0acb749783a55f13acf3`——**字节级不变**，它根本不需要重编译。项目里一个核心头被 300 个 TU 包含时，这个"0"就是分钟级和秒级的差别。

---

## 核心规则：析构（和移动）必须在 .cpp 定义

把头文件里的 `~Widget();` 声明删掉（或写成 `~Widget() = default;`），clang 报错原文：

```
error: invalid application of 'sizeof' to an incomplete type 'Widget::Impl'
    static_assert(sizeof(_Tp) >= 0, "cannot delete an incomplete type");
note: in instantiation of member function 'std::unique_ptr<Widget::Impl>::~unique_ptr' requested here
    ~Widget() = default;      // ← 错误示范：在头文件里 default
note: forward declaration of 'Widget::Impl'
```

机制：`unique_ptr<Impl>` 的默认删除器要 `delete Impl`，而 `delete` 需要 `sizeof(Impl)` 和 `Impl` 的析构——**都要求完整类型**。如果 `~Widget` 在头文件隐式生成，那么每个包含该头文件、且销毁 `Widget` 的 TU 都会在"Impl 不完整"的上下文里实例化这个析构，全军覆没。

**移动构造和移动赋值也一样**。实测把它们 `= default` 写在类内、用户 TU 里 Impl 不完整：

```
error: invalid application of 'sizeof' to an incomplete type 'W1::Impl'
note: in instantiation of member function 'std::unique_ptr<W1::Impl>::~unique_ptr' requested here
    W1(W1&&) = default;
error: invalid application of 'sizeof' to an incomplete type 'W2::Impl'
note: in instantiation of member function 'std::unique_ptr<W2::Impl>::operator=' requested here
    W2& operator=(W2&&) = default;
```

移动赋值要先删除旧值（触发删除器），移动构造在 libc++ 里也会实例化析构。口诀：**凡是会实例化删除器或创建 Impl 的函数，都得待在 Impl 完整的 TU。**

| 成员函数 | 头文件 | .cpp |
|---|---|---|
| 析构 `~Widget()` | 只声明 | `= default`（Impl 完整，安全） |
| 移动构造/移动赋值 | 只声明 | `= default` |
| 构造函数 | 只声明 | 定义（要 `make_unique<Impl>`） |
| 拷贝构造/拷贝赋值 | 通常 `= delete` | 要支持得手写深拷贝（Impl 逐成员复制） |

注意 `.cpp` 里 `~Widget() = default;` 是对的——"在类外 `= default`"让编译器在**这个 TU**（Impl 已完整）里生成析构体，头文件里 `= default` 才是错的。另外：如果同一个 TU 里 `Impl` 恰好已定义（单文件 demo），类内 `= default` **能编过**——但那等于没做 Pimpl，别拿单文件 demo 通过来证明规则不存在。

---

## 运行期成本：一次分配 + 一次间接

实测（`-O2`，`Direct` 的成员在对象里，`Pimpl` 的成员在堆上；方法定义在另一个 TU 保证是真实跨 TU 调用）：

```
sizeof(Direct) = 16   sizeof(Pimpl) = 8
构造+fill+析构 1000000 次:  Direct 12.9164 ms   Pimpl 110.309 ms   比值 8.54025x
sum() 20000000 次:  Direct 75.1448 ms  Pimpl 75.0923 ms   比值 0.999301x
  每次: Direct 3.75724 ns   Pimpl 3.75462 ns
```

两个结论，都值得记住：

1. **构造代价真实且很大**：8.5 倍，原因是每构造一个对象多一次 `make_unique<Impl>` 的堆分配。这就是"Pimpl 不适合热路径数据结构"的量化理由——订单、tick 每秒百万次构造，8.5 倍就是 8.5 倍。
2. **访问代价在 L1 命中时几乎测不出来**：20M 次 `sum()` 比值 0.999，多出来的那次 `pImpl->` 被乱序执行和 L1 命中完全隐藏。真正的风险不在稳态带宽，而在**冷 cache 时多一次可能的 cache miss** 和**跨 TU 无法内联**。

所以成本结构是：**主要在构造（堆分配），不在访问（间接）。** 判断标准也就清楚了——**对象构造得少、活得久，Pimpl 白赚；对象构造得频繁，Pimpl 血亏。**

---

## unique_ptr vs shared_ptr 版本

```cpp
// shared_ptr 版：连析构声明都可以省
class WidgetS {
    struct Impl;
    std::shared_ptr<Impl> pImpl;
public:
    WidgetS();            // 只有构造函数需要在 .cpp
    int size() const;
    void add(int v);
};
```

实测输出 `sizeof(WidgetS) = 16`、`size = 2`、`moved size = 2`——隐式生成的析构、移动全部能编译，不需要任何 out-of-line 声明。原因是 `shared_ptr` 的删除器在**构造时**就类型擦除绑定进控制块了，析构时走的是擦除后的函数指针，不要求 `Impl` 完整。

三条对比：`sizeof(Widget)` **8 vs 16**；析构/移动**必须** out-of-line vs 不需要；拷贝语义——`unique_ptr` 版默认不可拷贝（这是对的），`shared_ptr` 版**默认可拷贝但是浅拷贝**。

最后一条是 `shared_ptr` 版最隐蔽的坑：它"意外地"让 Pimpl 类变得可拷贝，而那个拷贝是共享 Impl 的假拷贝——两个 Widget 改一个影响另一个，几乎总是 bug。Pimpl 语境下所有权明明独占，`unique_ptr` 才是对价的工具——`shared_ptr` 版"能跑但买贵了"，还附赠一个语义陷阱。

---

## 什么时候不该用 Pimpl

**1. 模板类。** 模板成员必须在使用点实例化，`Impl` 的定义藏不进 `.cpp`。实测：把 `Holder<T>::Impl` 和成员定义都放 `.cpp` 而**不加**显式实例化，链接报 `Undefined symbols for architecture arm64` / `linker command failed with exit code 1`；加了 `template class Holder<int>;` 之后才通过（输出 `h.get() = 42 sizeof=8`）。也就是说每支持一个类型都要手写一行，编译防火墙的收益基本被抵消。

**2. 性能热路径的数据结构。** 实测构造成本 8.5 倍，加上跨 TU 不能内联。订单、行情 tick、定长环形缓冲这类每秒百万次构造的对象**绝不 Pimpl**——多一次堆分配就是多一次分配器锁。

**3. 小类 / 值语义类。** 只有两三个 `int` 成员的 `struct`，Pimpl 之后每对象多一次分配、代码量翻倍、还失去拷贝语义，纯亏。

**适合的是**：对外接口类——引擎、连接、会话、SDK 对象，特征是**构造少、活得久、接口稳定、实现会变**。

---

## 完整可运行例子

给上面的 `Widget` 补上**深拷贝**语义（单文件版便于验证；项目里同样要把这些定义放进 `widget.cpp`）：

```cpp
class Widget {
    struct Impl;
    std::unique_ptr<Impl> pImpl;
public:
    Widget();
    ~Widget();
    Widget(const Widget& o);                 // 深拷贝：定义在 Impl 完整处
    Widget& operator=(const Widget& o);
    Widget(Widget&&) noexcept;
    Widget& operator=(Widget&&) noexcept;
    void add(int v);
    int  sum() const;
};

struct Widget::Impl { std::vector<int> data; };

Widget::Widget() : pImpl(std::make_unique<Impl>()) {}
Widget::~Widget() = default;
Widget::Widget(const Widget& o) : pImpl(std::make_unique<Impl>(*o.pImpl)) {}  // 逐成员复制
Widget& Widget::operator=(const Widget& o) { *pImpl = *o.pImpl; return *this; }
Widget::Widget(Widget&&) noexcept = default;
Widget& Widget::operator=(Widget&&) noexcept = default;
void Widget::add(int v) { pImpl->data.push_back(v); }
int  Widget::sum() const { int s = 0; for (int v : pImpl->data) s += v; return s; }

int main() {
    Widget a; a.add(1); a.add(2);
    Widget b = a;          // 深拷贝
    b.add(100);
    std::cout << "a.sum()=" << a.sum() << "  b.sum()=" << b.sum() << "\n";
    Widget c; c = b;       // 拷贝赋值
    std::cout << "c.sum()=" << c.sum() << "\n";
}
```

编译运行（本机实测 clang 23.1.0，`-std=c++17 -Wall`）：

```
a.sum()=3  b.sum()=103
c.sum()=103
```

`b.add(100)` 之后 `a.sum()` 仍是 3 —— 两个 `Widget` 各有一份 `Impl`，这才是正确的拷贝语义（对比 `shared_ptr` 版的假拷贝）。

---

## 常见错误（新手踩坑）

1. **头文件里 `~Widget() = default;`**。最常见的 Pimpl 错误——`default` 在类内 = 在 Impl 不完整处生成析构体 = 每个使用者 TU 都编不过，报 `invalid application of 'sizeof' to an incomplete type`。记住口诀：**声明在头，default 在 .cpp**。
2. **移动成员忘了也去 .cpp**。只把析构挪走、移动构造留在类内隐式生成——实测移动构造和移动赋值都会触发删除器实例化，同样炸。
3. **想要拷贝又忘了深拷贝**。Pimpl 类默认不可拷贝，要拷贝语义需在 .cpp 手写 `Widget::Widget(const Widget& o) : pImpl(std::make_unique<Impl>(*o.pImpl)) {}`（实测深拷贝版 `b.add(100)` 后 `a.sum()` 仍是 3）。
4. **用 `shared_ptr` 做 Pimpl 还以为拷贝是对的**。它让类意外可拷贝，且拷出来是共享 Impl 的浅拷贝。
5. **在热路径类上用 Pimpl**。实测构造成本 8.5 倍，tick 数据结构用 Pimpl 是把编译时间省下来、把延迟花出去。

---

## HFT 关联

- **交易引擎的编译时间**：核心引擎头文件被几百个策略 TU 包含，改一行私有实现全量重编分钟级。引擎主类 Pimpl 化后，策略侧只依赖稳定接口头，增量编译秒级——实测单个 TU 的预处理量从 35798 行降到 13531 行。
- **商业库交付**：给外部团队的 SDK（行情接入、风控前置）用 Pimpl 保持 ABI 稳定——你升级实现，对方不重编，实测改完 `widget.cpp` 后用户 `.o` 的 md5 字节级不变。8 字节的固定对象布局是合同的一部分。
- **热路径反例**：订单、行情 tick 这类每秒百万级构造/访问的对象**绝不 Pimpl**——实测构造成本 8.5 倍，多一次堆分配就是多一次分配器锁。Pimpl 是给"少而重"的接口类的。
- **网关/会话层正例**：`Session`、`MarketDataFeed`、`RiskGateway` 这类"进程启动时建几个、跑一整天"的对象，Pimpl 的收益远大于那一次构造期的堆分配。

---

## 自测题

**1. Pimpl 里为什么析构必须在 `.cpp` 定义？头文件里 `= default` 会报什么错？**
<details><summary>答案</summary>
`unique_ptr<Impl>` 的默认删除器要 `delete Impl`，需要 `sizeof(Impl)` 和 Impl 的析构——都要求完整类型。析构若在头文件（类内）隐式生成，每个包含头文件并销毁 Widget 的 TU 都在 Impl 不完整的上下文实例化它。实测报 `error: invalid application of 'sizeof' to an incomplete type 'Widget::Impl'`（libc++ 里是 `static_assert(sizeof(_Tp) >= 0, "cannot delete an incomplete type")`）。
</details>

**2. 除了析构，还有哪些特殊成员函数必须挪到 `.cpp`？为什么？**
<details><summary>答案</summary>
移动构造和移动赋值（实测两者都会触发删除器实例化，报同样的 `sizeof to an incomplete type`）；构造函数也要在 .cpp（`make_unique<Impl>` 需要完整类型，报 `allocation of incomplete type`）；若要拷贝语义，拷贝构造/赋值也要在 .cpp 手写深拷贝。规律：**凡是会实例化删除器或创建 Impl 的函数，都得待在 Impl 完整的 TU。**
</details>

**3. 编译防火墙的收益能用哪些数字量化？实测值是多少？**
<details><summary>答案</summary>
①预处理后行数：非 Pimpl 35798 行 vs Pimpl 13531 行（2.6x）；②`-fsyntax-only` 编译耗时 0.270 s vs 0.120 s（2.25x）；③最硬的指标——改完 `widget.cpp` 里的 Impl（加 `<map>`、`<string>` 依赖）重编 `widget.o`，用户 `main.o` 的 **md5 字节级不变**，可以直接复用重链接。第三条说明用户 TU 根本不需要重编译。
</details>

**4. Pimpl 的运行期成本实测是多少？主要体现在哪？**
<details><summary>答案</summary>
实测 `-O2`：构造+fill+析构 100 万次，Direct 12.9 ms vs Pimpl 110.3 ms（**8.5 倍**）；`sum()` 2000 万次，Direct 3.757 ns/次 vs Pimpl 3.755 ns/次（**比值 0.999，几乎无差**）。结论：成本主要在**构造时的一次堆分配**，而不在访问时的一次间接寻址——后者在 L1 命中时被乱序执行完全隐藏。所以判断标准是"构造频率"而非"访问频率"。
</details>

**5. 用 `shared_ptr<Impl>` 做 Pimpl 为什么能省掉析构声明？为什么不推荐？**
<details><summary>答案</summary>
`shared_ptr` 的删除器在**构造时**类型擦除绑定到控制块，析构时走擦除后的函数指针，不要求 `Impl` 完整——实测 `WidgetS` 只有构造函数声明在 .cpp，隐式生成的析构/移动全部能编译。不推荐的原因有二：①Pimpl 场景所有权明明独占，`shared_ptr` 白付原子引用计数的税，`sizeof` 从 8 变 16（实测）；②它让类**意外变得可拷贝**，而那个拷贝是共享同一个 Impl 的浅拷贝，改一个影响另一个，几乎总是 bug。
</details>

**6. 模板类用 Pimpl 会遇到什么？实测报什么错？**
<details><summary>答案</summary>
模板成员必须在使用点实例化，`Impl` 定义藏不进 `.cpp`。若把 `Holder<T>::Impl` 和成员函数都放在 `.cpp` 而**不加显式实例化**，实测链接报 `Undefined symbols for architecture arm64` / `linker command failed with exit code 1`；加上 `template class Holder<int>;` 之后链接通过（实测输出 `h.get() = 42 sizeof=8`）。也就是说每支持一个类型都要手写一行显式实例化，编译防火墙的收益基本被抵消。
</details>

**7. Pimpl 和 C 的 opaque pointer 是什么关系？C++ 版升级了什么？**
<details><summary>答案</summary>
Pimpl 就是 C opaque pointer 惯用法的 C++ 版：C 里头文件只给 `typedef struct Widget Widget;`，实现藏 `.c`。目的相同（隐藏实现、稳定 ABI）。C++ 版的升级：①`unique_ptr` 自动管理释放，异常路径不泄漏，不用祈祷调用方记得调 `Widget_destroy`；②类成员函数语法保留，接口仍是一个 class 而不是一包 `Widget_xxx(Widget*)` 自由函数。代价是 C 版没有的那一次堆分配（C 版同样是 `malloc`——所以这点上两者持平）。
</details>

---

## 参考与延伸

- 上一节：[Item 21 make 函数](item21-make-functions.md)
- 下一章：[第 5 章 右值引用、移动语义与完美转发](../ch05-rvalue-move-forwarding/README.md)
- 相关：[Item 18 unique_ptr](item18-unique-ptr.md)（不完整类型 + 删除器为什么要求完整类型）
- 回到：[第 4 章 智能指针](README.md)
