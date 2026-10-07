# 条款 23：优先用非成员、非友元函数替代成员函数

## 本节讲什么

**Prefer non-member non-friend functions to member functions.** 这条违反直觉：
"功能放类外"居然比"放类里"**更封装**？Meyers 的论证干净利落——
封装的本义是"能看到内部状态的代码越少越好"，非成员非友元函数**碰不到 private**，
所以把能挪出类的功能挪出去，类就更小、更稳。标准库 `<algorithm>` 就是这套哲学的示范。

← 上一条 [item22 成员变量一律 private](./item22-成员变量一律声明为private.md)；
下一条 [item24 非成员函数支持全参数转换](./item24-需要所有参数都支持隐式类型转换时，使用非成员函数.md)。

---

## 1. 封装性论证：能碰 private 的代码越少，封装越强

```cpp
class OrderBook {
    std::vector<Level> bids_, asks_;        // private
public:
    void add(const Order& o);
    void cancel(uint64_t id);
    // 核心接口到此为止——越小越好

    // ❌ 这些"便利功能"如果也做成成员，每一个都能碰 bids_/asks_：
    // void clear_both_sides();
    // void print_top(int n) const;
    // double mid_price() const;
};

// ✅ 便利功能放类外，同一命名空间——只能通过公开接口工作：
namespace orderbook {
    void clear_both_sides(OrderBook& ob) { /* 只用公共 API */ }
    double mid_price(const OrderBook& ob) { return (ob.best_bid() + ob.best_ask()) / 2.0; }
}
```

**关键推理**：`clear_both_sides` 作为成员函数时，它**有权**直接改 `bids_`——
即使今天它规规矩矩走公共接口，明天某个"优化"就可能绕过不变量直接捅数据。
放类外后，它**物理上不可能**破坏封装——编译器替你守。

**封装度公式**（Meyers）：封装性 ∝ 1 / 能访问 private 的代码量。
成员函数 + 友元函数都算"能访问的"——非成员非友元函数不算。

## 2. 标准库的示范：算法全在类外

```cpp
std::vector<int> v = {3, 1, 4};
std::sort(v.begin(), v.end());          // sort 不是 vector 的成员！
auto it = std::find(v.begin(), v.end(), 4);   // find 也不是
```

为什么 `sort` 不做成 `v.sort()`？
- vector 的核心职责是**存储**（大小/容量/元素访问）——排序与它无关
- 算法放类外：N 个容器 × M 个算法 **不需要 N×M 个成员函数**，
  迭代器抽象让 M 个算法服务所有容器
- 若 sort 是成员：每种容器各写一遍 sort（array 有、vector 有、deque 有……）——
  代码爆炸且无法复用到自定义容器

**设计推论**：类的核心接口做到**最小完备**（用户能完成一切，但没有一行多余）；
其余全是便利函数，放同一命名空间的自由函数。

## 3. 什么时候必须是成员/友元（边界）

| 必须成员 | 必须友元（尽量免） | 应放类外 |
|---|---|---|
| 构造函数/析构/赋值 | 流运算符（左操作数是 stream） | 一切能用公共 API 实现的功能 |
| 虚函数（多态） | 需要碰多个类 private 的运算 | 便利函数/格式化/统计 |
| `operator[]/()/->/=` | | 不改变对象状态的分析函数 |
| 维护不变量的状态变更 | | 组合多个公共操作的流程 |

**组织手法**（同一命名空间 + 分头文件）：

```cpp
// orderbook_core.h——核心类，最小接口
namespace orderbook { class OrderBook { /*...*/ }; }

// orderbook_utils.h——便利函数（想用才 include，依赖单向）
namespace orderbook {
    double mid_price(const OrderBook&);
    void print_depth(const OrderBook&, int levels);
}
```

用户只 include core 就能干活；utils 不污染核心头，编译依赖也解开了。

## HFT 关联

- **订单簿/撮合引擎的核心类必须极小**：热路径类每多一个成员函数，
  就多一个能破坏不变量的入口（"顺手改下 bids_" 的诱惑）——
  统计/打印/回放分析全放类外，核心类只留 add/cancel/execute
- 非成员便利函数天然**可独立测试**：`mid_price(ob)` 不需要 mock 整个类
- 与 item22 配套：private（22）+ 最小核心接口（23）+ 全 static_assert 协议（19.5）
  = 热路径类型的三层防线

## 代码自测

**题目 1：** "把功能挪出类"为什么反而**增强**封装？它绕过了什么？

<details>
<summary>参考答案</summary>

封装 = 限制"能看到内部状态的代码量"。成员函数和友元函数都有权访问 private——
每多一个这样的函数，封装就被多打开一个口子（即使它今天很规矩）。
非成员非友元函数**物理上无法**访问 private，只能通过公共接口工作：
功能照样提供，但能破坏不变量的代码量不增反减。
它不是"绕过"什么，而是让编译器把"不许碰内部"变成强制约束。

</details>

**题目 2：** `std::sort` 不做成 `vector::sort()` 的真实收益是什么？

<details>
<summary>参考答案</summary>

① 正交分解：vector 管存储，算法管逻辑——N 容器 × M 算法不需要 N×M 个成员；
② 复用：sort 经迭代器服务一切容器（含你自定义的环形缓冲）；
③ 编译解耦：不用 sort 的 TU 不为它付编译成本；
④ 核心类最小化：vector 的接口少了几十个算法成员，封装面缩小。
若 sort 是成员：每种容器各实现一遍 sort，且新增算法要改每个容器类。

</details>

**题目 3：** 哪些函数**不能**挪出类？给四个类别并各举一例。

<details>
<summary>参考答案</summary>

① 特殊成员函数：构造/析构/赋值（语言规定必须是成员）；
② 虚函数：多态必须挂在类上（`virtual void on_tick()`）；
③ 特定运算符：`operator=`/`[]`/`()`/`->`（语言要求成员）；
④ 维护不变量的状态变更：`OrderBook::add` 必须保证插入后仍有序——
放类外就守不住 private 不变量（除非全走公共接口，但那样公共接口又变成了变相的全开放）。
判别：碰 private 且要维护不变量 → 成员；能用公共 API 实现 → 类外。

</details>
