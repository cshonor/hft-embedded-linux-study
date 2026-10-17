# 条款 31：降低文件之间的编译依赖

## 本节讲什么

**Minimize compilation dependencies between files.** C++ 的 `#include` 是文本包含——
头文件里任何改动（包括私有成员）都让所有包含者重编。本条款两件武器：
**能用声明就不用定义**（前置声明的七种场景）、**把实现藏到指针后面**
（pImpl / interface class）。这是 19.4（pImpl 机制）的工程总纲。

← 上一条 [item30 内联的里里外外](./item30-透彻理解内联inline的优缺点.md)；
下一章 [ch06 继承与面向对象](../ch06-inheritance-oop/)。

---

## 1. 问题：C++ 的接口/实现不分离

```cpp
// order.h
#include <string>
#include "price.h"          // price.h 改一个字 → 所有 include order.h 的文件重编
#include "qty.h"

class Order {
    std::string symbol_;    // 私有成员的**定义**也在头文件里——
    Price price_;           // 换个成员类型/加个成员，用户代码全部重编
    Qty qty_;
};
```

C/C++ 的编译模型决定了：**类的定义必须包含成员的完整布局**（sizeof 要在每个
使用点算出来）——所以"私有实现"在编译期其实毫无隐私，改动传染面极大。
大型项目里改一个底层头文件触发几千个 TU 重编，就是这条的代价。

## 2. 武器一：前置声明（forward declaration）的七种场景

**能用 `class X;` 就不用 `#include "x.h"`**——不完整类型（incomplete type）的合法用途：

| 场景 | 能用前置声明吗 |
|---|---|
| 声明指针/引用成员（`X* p_; X& r_;`） | ✅ |
| 函数参数/返回值是 X（**仅声明**） | ✅ |
| 函数参数/返回值是 X（**定义函数体**要用到 X 的成员） | ❌ 需要完整定义 |
| **按值**成员变量（`X x_;`——要算 sizeof） | ❌ |
| 继承自 X（要完整布局） | ❌ |
| 实例化以 X 为参数的模板（`vector<X>`——保守起见需要） | ⚠️ 多数情况需要 |
| `sizeof(X)`、`new X`、`X::static_member` | ❌ |

**实践**：头文件里只 include"成员按值持有/继承"的——其余一律前置声明，
把 `#include` 挪到 .cpp。顺带收益：头文件变轻 → 编译更快。

## 3. 武器二：pImpl（handle class）——把实现整个藏到指针后

```cpp
// order.h——稳定如石：加成员/改实现都不再触发重编
class Order {
    class Impl;                       // 前置声明嵌套类（→ 19.4 ③）
    std::unique_ptr<Impl> impl_;
public:
    Order(); ~Order();                // 在 .cpp 定义（Impl 完整后才能析构）
    Order(Order&&) noexcept;
    void submit();
};

// order.cpp——实现细节的自由天地
class Order::Impl {
    std::string symbol_; Price price_; Qty qty_;   // 随便改，用户无感
};
```

**代价**：每次成员访问多一次指针间接 + 一次堆分配——
所以 pImpl 用在**接口边界/编译热点**（被几百个文件包含的基础类），
不用于热路径数据类型（→ 19.4 HFT 关联）。

## 4. 武器三：interface class（抽象基类）

```cpp
class IOrderGateway {                  // 纯虚接口：头文件几乎永不变
public:
    virtual ~IOrderGateway() = default;
    virtual void send(const Order&) = 0;
    virtual void cancel(uint64_t id) = 0;
};
// 用户依赖 IOrderGateway；实现类（FixGateway/BinaryGateway）在 .cpp 里——
// 工厂函数返回 unique_ptr<IOrderGateway>
```

- 接口类没有数据成员 → 没有布局依赖 → **终极编译防火墙**
- 同时拿到运行期多态的可替换性（mock/多交易所适配）
- 代价：虚调用间接 + 堆分配——同样留给冷路径边界

## 5. 三武器的选用（决策表）

| 你的痛点 | 武器 |
|---|---|
| 头文件 include 链太长、编译慢 | 前置声明（成本最低，先做） |
| 基础类的实现经常改、牵连面广 | pImpl |
| 需要可替换实现（mock/多后端） | interface class |
| 热路径数据类型（Tick/Order） | **都不用**——布局稳定 + static_assert 钉死（→ 19.5） |

## HFT 关联

- 交易系统的编译热点：**协议结构体头文件**（被全仓包含）——
  这些头文件必须"布局稳定 + 零依赖"（只含 POD + static_assert），
  任何辅助函数挪到 utils 头（→ item23 分头文件手法）
- 策略框架的**接口边界**（Strategy/Engine 之间）用 interface class：
  策略重编不触发引擎重编，回测/实盘共享同一策略二进制
- 前置声明 + `unique_ptr<Impl>` 是行情网关（FeedHandler）的标准形态：
  协议升级只改 .cpp，策略层零重编（→ 18.2 inline namespace 版本化配套）

## 代码自测

**题目 1：** 为什么"只 include 你需要的，前置声明其余的"能显著降低编译依赖？

<details>
<summary>参考答案</summary>

`#include` 是文本包含——被包含文件的任何改动（哪怕注释/私有成员）都让包含者的
预处理结果变化，触发重编。前置声明 `class X;` 不引入任何文本内容：
X 的头文件再怎么改，依赖它的这个头文件**纹丝不动**——依赖被推迟到
"真正需要完整定义"的那个 .cpp。把头文件的 include 最小化，等于把
"重编传染面"从图论上的稠密图剪成稀疏图。

</details>

**题目 2：** pImpl 的 `unique_ptr<Impl>` 成员下，为什么析构函数必须在 .cpp 定义？

<details>
<summary>参考答案</summary>

`unique_ptr<Impl>` 析构时要 `delete impl_`——这要求 Impl 在析构点是**完整类型**。
头文件里 Impl 只有前置声明（不完整），若析构在头文件 `= default`，
每个包含者编译到 `~Order()` 时都面对不完整 Impl → 报错
（"invalid application of sizeof to incomplete type"）。
在 .cpp（Impl 完整定义处）定义析构，所有 TU 链接同一份。
（同理：拷贝构造/赋值要自定义或禁用——默认版需要完整类型做深拷贝。）

</details>

**题目 3：** 热路径的 `Tick` 结构体为什么不适用 pImpl？该用什么策略管理它的编译依赖？

<details>
<summary>参考答案</summary>

pImpl 的代价——每次访问一次指针间接（潜在 cache miss）+ 一次堆分配——
在 tick-to-trade 预算里不可接受；且 Tick 的价值就是"布局 = 线上字节"，
藏起来反而失去了零拷贝解码的前提（→ 19.5）。
Tick 的编译依赖管理策略是**稳定化**而非隐藏化：
① 布局钉死（static_assert 三件套，改动必须走协议评审）；
② 头文件零依赖（只含 POD + 定宽类型，不 include 业务头）；
③ 辅助函数（解析/格式化）挪到独立 utils 头——核心头文件永不变，
"依赖"自然无从传染。

</details>
