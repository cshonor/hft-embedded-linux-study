# Item 22：用 Pimpl 惯用法降低编译依赖

> 第 4 章 智能指针 · Item 22 · 上一节：[Item 21 make 函数](item21-make-functions.md)

## 为什么要学这个（先建立直觉）

C++ 的编译模型有个结构性痛点：头文件里类的**私有成员**也是类定义的一部分——你改一行私有实现（加个 `vector` 成员、换个第三方库），所有包含这个头文件的翻译单元全部重编译。大项目里改一个核心头文件触发几百个 TU 重编，"喝杯咖啡等编译"就是这么来的。

Pimpl（Pointer to Implementation）的思路釜底抽薪：头文件里只留一个指向实现结构的指针，实现整体挪进 `.cpp`。私有成员随便改，只有那一个 `.cpp` 重编译。代价是多一次间接和多一次堆分配——所以它是**接口类的工具**，不是热路径数据结构的工具。

---

## 这节讲什么

1. Pimpl 的标准结构（头文件 + `.cpp` 两侧完整代码，实测可编译）；
2. 核心规则：**析构为什么必须在 `.cpp` 定义**（含报错原文）；
3. `unique_ptr` vs `shared_ptr` 做 Pimpl 的区别；
4. 成本核算：收益（编译防火墙、ABI 稳定）与代价（间接、堆分配）。

---

## 核心结构（完整可编译）

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
    int get() const;
};

// ============ widget.cpp ============
#include "widget.h"
#include <vector>

struct Widget::Impl {               // 实现细节全在这里
    std::vector<int> data{1, 2, 3};
};

Widget::Widget()  : pImpl(std::make_unique<Impl>()) {}
Widget::~Widget() = default;        // Impl 在此处已完整，default 安全
Widget::Widget(Widget&&) noexcept = default;
Widget& Widget::operator=(Widget&&) noexcept = default;
int Widget::get() const { return pImpl->data.size(); }

// ============ main.cpp ============
#include "widget.h"
int main() {
    Widget w;                    // w.get() == 3
    Widget w2 = std::move(w);    // 移动 OK
    // sizeof(Widget) == 8 —— 就装一个指针
}
```

clang 23.1.0 实测编译运行通过：`w.get() = 3`、`sizeof(Widget) = 8`、移动后 `w2.get() = 3`。

**调用方视角的关键数字：`sizeof(Widget) = 8`**。不管 `Impl` 里塞多少成员（vector、map、第三方库对象），`Widget` 永远是一个指针的大小——这正是 ABI 稳定的来源。

---

## 核心规则：析构（和移动）必须在 .cpp 定义

把上面头文件里的 `~Widget();` 声明删掉（让编译器在类内隐式生成），clang 报错原文：

```
error: allocation of incomplete type 'Widget::Impl'
error: invalid application of 'sizeof' to an incomplete type 'Widget::Impl'
```

机制：`unique_ptr<Impl>` 的默认删除器要 `delete Impl`，而 `delete` 需要 `sizeof(Impl)` 和 `Impl` 的析构——**都要求完整类型**。如果 `~Widget` 在头文件隐式生成，那么每个包含该头文件、且销毁 `Widget` 的 TU 都会在"Impl 不完整"的上下文里实例化这个析构，全军覆没。

对策就是把会触发删除器的特殊成员函数**声明在头文件、定义在 .cpp**（Impl 完整之后）：

| 成员函数 | 头文件 | .cpp |
|---|---|---|
| 析构 `~Widget()` | 只声明 | `= default`（Impl 完整，安全） |
| 移动构造/移动赋值 | 只声明 | `= default` |
| 构造函数 | 只声明 | 定义（要 `make_unique<Impl>`） |
| 拷贝构造/拷贝赋值 | 通常 `= delete` | 要支持得手写深拷贝（Impl 逐成员复制） |

注意 `.cpp` 里 `~Widget() = default;` 是对的——"在类外 `= default`"让编译器在**这个 TU**（Impl 已完整）里生成析构体。头文件里 `= default` 才是错的。

### 用 shared_ptr 做 Pimpl 的例外

`shared_ptr<Impl>` 的删除器是**构造时类型擦除绑定**的，析构时不需要 `Impl` 完整——所以 `shared_ptr` 版 Pimpl 可以连析构声明都省掉。但代价是引用计数开销 + `sizeof(Widget)` 变 16。Pimpl 语境下所有权明明独占，`unique_ptr` 才是对价的工具，`shared_ptr` 版本属于"能跑但买贵了"。

---

## 成本核算

**收益：**

1. **编译防火墙**：头文件不再 `#include` 实现依赖（`<vector>`、第三方库头）。改 `Impl` 只重编一个 `.cpp`；包含头文件的几百个 TU 不被传染。
2. **ABI 稳定**：`Widget` 对象布局恒为一个指针（实测 8 字节）。库升级改了私有实现，调用方**不用重编译**，链上新 .so/.a 就行——动态库发布的根基技术。
3. **信息隐藏**：私有成员的定义彻底不进头文件，连"看到了但访问不了"都不存在。

**代价：**

1. 每次访问实现多一次指针间接（`pImpl->`）；
2. 每个对象多一次堆分配（`make_unique<Impl>`）；
3. 代码量翻倍（声明/定义分离，Impl 里还要搬一份成员函数）。

**适用判断**：对外接口类（引擎、连接、会话——构造少、活得久、接口稳定）→ 值；热路径数据结构（订单、行情 tick——每秒百万次构造/拷贝/访问）→ 不值，间接和堆分配都是热路径毒药。

---

## 常见错误（新手踩坑）

1. **头文件里 `~Widget() = default;`**。最常见的 Pimpl 错误——`default` 在类内 = 在 Impl 不完整处生成析构体 = 每个使用者 TU 都编不过。记住口诀：**声明在头，default 在 .cpp**。
2. **移动成员忘了也去 .cpp**。只把析构挪走、移动构造留在类内隐式生成——`unique_ptr` 的移动赋值会先析构旧值，同样触发删除器实例化，同样炸。
3. **想要拷贝又忘了深拷贝**。`unique_ptr` 不可拷贝，Pimpl 类默认不可拷贝。要拷贝语义需在 .cpp 手写：`Widget::Widget(const Widget& o) : pImpl(std::make_unique<Impl>(*o.pImpl)) {}`。
4. **在热路径类上用 Pimpl**。每次访问多一次间接 + 每对象一次堆分配——tick 数据结构用 Pimpl 是把编译时间省下来、把延迟花出去。

---

## 新手要点（和 C 的区别）

- **C 的 opaque pointer 就是 Pimpl 的祖宗**：头文件 `typedef struct Widget Widget;`，`.c` 里 `struct Widget { ... };`，调用方只摸指针。《C 和指针》ch07 讲过这个手法。C++ 版的升级是 `unique_ptr` 自动管理生命周期——C 版要手写 `Widget_create/Widget_destroy` 并祈祷调用方记得调。
- **为什么 C++ 比 C 更需要它**：C 的 struct 成员可以藏在 `.c` 里天经地义；C++ 的 class 私有成员必须在头文件的类定义里——Pimpl 是把 C 的隐藏能力"偷渡"回 C++ 的办法。
- **判断口诀**：这个类是"给别人用的接口"还是"热路径上的数据"？前者考虑 Pimpl，后者千万别。

---

## HFT 关联

- **交易引擎的编译时间**：核心引擎头文件被几百个策略 TU 包含，改一行私有实现全量重编分钟级。引擎主类 Pimpl 化后，策略侧只依赖稳定接口头，增量编译秒级。
- **商业库交付**：给外部团队的 SDK（行情接入、风控前置）用 Pimpl 保持 ABI 稳定——你升级实现，对方不重编。8 字节的固定对象布局是合同的一部分。
- **热路径反例**：订单、行情 tick 这类每秒百万级构造/访问的对象**绝不 Pimpl**——多一次间接就是多一次潜在 cache miss，多一次堆分配就是多一次分配器锁。Pimpl 是给"少而重"的接口类的。

---

## 自测题

1. Pimpl 里为什么析构必须在 `.cpp` 定义？头文件里 `= default` 会报什么错？
2. 除了析构，还有哪些特殊成员函数必须挪到 `.cpp`？为什么？
3. 实测 `sizeof(Widget)`（Pimpl 版）是多少？这个数字为什么就是 ABI 稳定的来源？
4. 用 `shared_ptr<Impl>` 做 Pimpl 为什么能省掉析构声明？为什么不推荐？
5. Pimpl 的三项代价是什么？什么样的类适合/不适合？
6. Pimpl 和 C 的 opaque pointer 是什么关系？C++ 版升级了什么？

<details>
<summary>参考答案</summary>

1. `unique_ptr<Impl>` 的默认删除器要 `delete Impl`，需要 `sizeof(Impl)` 和 Impl 的析构——都要求完整类型。析构若在头文件（类内）隐式生成，每个包含头文件并销毁 Widget 的 TU 都在 Impl 不完整的上下文实例化它，编译失败：`error: invalid application of 'sizeof' to an incomplete type 'Widget::Impl'`。把 `~Widget();` 只声明在头文件、`.cpp` 里 `= default`（Impl 完整之后），析构体就只在一个 TU 生成。

2. 移动构造和移动赋值（`unique_ptr` 移动赋值会先析构旧值，同样触发删除器）；构造函数也要在 .cpp（`make_unique<Impl>` 需要完整类型，报 `allocation of incomplete type`）；若要拷贝语义，拷贝构造/赋值也要在 .cpp 手写深拷贝。规律：**凡是会实例化删除器或创建 Impl 的函数，都得待在 Impl 完整的 TU**。

3. 8 字节（实测）——一个 `unique_ptr` 指针。无论 Impl 里成员怎么增删改，`Widget` 的对象布局不变，已有调用方代码的二进制依旧合法：改实现只需重编/重链库一侧，调用方不用动。这就是"ABI 稳定"的物理含义。

4. `shared_ptr` 的删除器在**构造时**类型擦除绑定到控制块，析构时走的是擦除后的函数指针，不要求 `Impl` 完整——所以可以连析构声明都省。不推荐是因为 Pimpl 场景所有权明明独占：`shared_ptr` 白付原子引用计数的税，对象还变 16 字节。独占语义用 `unique_ptr`，这同时也是在向读者声明"不共享"。

5. 代价：访问实现多一次指针间接；每对象多一次堆分配；声明/定义分离 + Impl 搬运的代码量翻倍。适合：构造少、活得久、接口稳定的对外接口类（引擎、连接、SDK 对象）。不适合：热路径数据结构（订单、tick）——间接是 cache miss，堆分配是分配器锁，都是延迟毒药。

6. Pimpl 就是 C opaque pointer 惯用法的 C++ 版：C 里头文件只给 `typedef struct Widget Widget;`，实现藏 `.c`。目的相同（隐藏实现、稳定 ABI）。C++ 版的升级：①`unique_ptr` 自动管理释放，异常路径不泄漏，不用祈祷调用方记得调 `Widget_destroy`；②类成员函数语法保留，接口仍是一个 class 而不是一包 `Widget_xxx(Widget*)` 自由函数。

</details>

---

## 参考与延伸

- 下一章：[第 5 章 右值引用、移动语义与完美转发](../ch05-rvalue-move-forwarding/README.md)
- 回到：[第 4 章 智能指针](README.md)
