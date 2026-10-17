# 条款 31：多重分派——根据两个以上对象的类型动态匹配

## 本节讲什么

**Implement double dispatch: virtual dispatch on two or more object types.**
虚函数按**一个**对象的类型分发（this）。当行为依赖**两个**对象的类型组合
（碰撞检测：行星×飞船？订单类型×风控规则？），单虚函数不够用了——
需要双重分派（double dispatch）。本条给出两种实现（转回分发/visit 矩阵）
与现代替代（variant×variant + std::visit）。

← 上一章 [ch06 智能指针](../ch06-smart-pointers/)；
下一条 [item32 面向未来设计](../ch07-miscellany/item32-面向未来做程序设计（兼容扩展、向后兼容设计思想）.md)。

---

## 1. 问题：行为 = f(类型A, 类型B)

```cpp
// 撮合结果 = f(主动单类型, 被动单类型)：
// IOC × Limit、IOC × IOC、FOK × Limit……每种组合规则不同
void match(const Order& aggressor, const Order& resting) {
    // aggressor.execute(resting)?——execute 只知道 this（主动单）的类型，
    // 被动单的类型在参数里，虚函数够不着
}
```

单虚函数只解决"我是 IOC 单"——"对方是什么单"还需要第二次分发。

## 2. 解法一：转回分发（re-dispatch / 双重虚调用）

```cpp
struct LimitOrder; struct IocOrder;

struct Order {
    virtual void execute_aggressor(Order& resting) = 0;          // 第一次分发：主动单类型
    virtual void accept_from(LimitOrder& agg) = 0;               // 第二次分发的接收口
    virtual void accept_from(IocOrder& agg) = 0;
    virtual ~Order() = default;
};
struct LimitOrder : Order {
    void execute_aggressor(Order& resting) override {
        resting.accept_from(*this);              // 转回：让对方按"我的类型"再接一次
    }
    void accept_from(LimitOrder&) override { /* Limit×Limit 规则 */ }
    void accept_from(IocOrder&) override { /* Ioc×Limit 规则 */ }
};
struct IocOrder : Order { /* 对称 */ };

// match(a, b) → a.execute_aggressor(b) [分发到 a 的类型]
//             → b.accept_from(a_as_Limit) [再分发到 b 的类型 + a 的具体类型]
// 两次虚调用后，组合 (A类型, B类型) 精确定位
```

**成本**：M 种类型 × M 个 accept_from 重载 = **M² 的矩阵**——
类型少（2-3 种）可控，类型多爆炸（→ 解法二/三）。

## 3. 解法二：visit 矩阵（查表版）

```cpp
using MatchFn = void(*)(Order&, Order&);
static MatchFn match_table[NUM_TYPES][NUM_TYPES];   // 显式矩阵：组合 → 处理函数
void match(Order& a, Order& b) {
    match_table[a.type()][b.type()](a, b);           // 两次数组索引，O(1)
}
```

**成本**：M² 表项（同解法一的组合数），但**分派是查表不是虚调用链**——
热路径友好（→ 19.3 成员指针表的二维版）。

## 4. 解法三（现代）：variant×variant + std::visit

```cpp
using AnyOrder = std::variant<LimitOrder, IocOrder, FokOrder>;
void match(AnyOrder& a, AnyOrder& b) {
    std::visit([](auto& agg, auto& rest) {
        if constexpr (std::is_same_v<decltype(agg), LimitOrder&> &&
                      std::is_same_v<decltype(rest), IocOrder&>) {
            // Limit×Ioc 规则——编译期精确定位，零虚调用
        } else { /* 通用规则 */ }
    }, a, b);
}
// visit 展开为 M×N 的编译期分派矩阵——组合精确、可内联、漏组合有编译诊断
```

类型集**封闭**时这是最优解（编译期矩阵 + if constexpr 精确到组合）；
类型集开放（运行时注册新订单类型）回到解法一/二。

## HFT 关联

- **撮合规则矩阵**是双重分派的教科书场景：主动单类型 × 被动单类型——
  生产实践是解法二（type 字段 + 二维查表）：规则矩阵**显式可见可评审**，
  分派 O(1)，比 M² 个虚函数的组合好维护
- 解法一的 M² 爆炸在订单类型 >3 时失控——交易所订单类型动辄十几种，
  这就是为什么"转回分发"教科书美但生产少用
- variant 版（解法三）在"订单类型封闭"的内部系统是最优：
  if constexpr 把组合规则**编译期内联**——热路径零间接

## 代码自测

**题目 1：** 为什么单个虚函数解决不了"行为依赖两个对象类型"？

<details>
<summary>参考答案</summary>

虚函数分发的依据是 **this 指向的对象**——`a.f(b)` 只能按 a 的类型
选实现，b 在函数签名里只是个**静态类型的参数**（编译期已知是 Order&，
运行期是什么派生类，虚函数机制不知道也不关心）。
要让 b 的类型也参与分发，必须**第二次**基于 b 的虚调用——
这就是"双重分派"必须人为构造的原因：
C++ 的多态原生只支持单维（单对象），第二维要靠
转回分发/查表/variant 手工补出来。

</details>

**题目 2：** 转回分发（re-dispatch）为什么类型多了会爆炸？爆炸的具体形式？

<details>
<summary>参考答案</summary>

每个具体类型都要为**所有**类型写 `accept_from(X&)` 重载——
M 种类型 = 每类 M 个重载 = **M² 个函数**。
3 种订单类型 = 9 个组合函数（可控）；10 种 = 100 个（维护灾难——
新增第 11 种要改全部 10 个现有类 + 新类的 11 个重载，违反开闭原则）。
这就是"访问者模式"（double dispatch 的学名）的核心痛点：
**类型稳定时它是利器，类型演化时它是税**——
所以生产撮合系统（订单类型常年演化）选查表矩阵（解法二），
把 M² 从"函数"变成"数据"（表项），加类型只加表项不改类。

</details>

**题目 3：** variant + visit 的双重分派相比虚函数转回，三个优势一个前提？

<details>
<summary>参考答案</summary>

优势① **编译期矩阵**：`std::visit` 展开为 M×N 的静态分派——
无 vtable、无虚调用，组合规则可内联（热路径零间接）；
优势② **精确到组合**：`if constexpr` 可以对"Limit×IOC"这一**具体组合**
写专属代码（虚函数转回要为每个类写一族 accept_from，
组合特化不如 if constexpr 直观）；
优势③ **漏组合有诊断**：visit 的分支缺失可静态检测
（overloaded 组合不全 = 编译错误）。
前提：**类型集封闭**——variant 的成员类型写死在声明里，
运行时注册新类型（插件订单类型）做不到；
那种场景回到转回分发（解法一）或查表（解法二）。
"封闭集 → variant 矩阵，开放集 → 虚函数/查表"是双重分派的选型总纲。

</details>
