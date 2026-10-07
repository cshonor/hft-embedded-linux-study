# 条款 38：通过复合塑模出 has-a 或"以某物实现"

## 本节讲什么

**Model "has-a" or "is-implemented-in-terms-of" through composition.**
复合（composition，成员变量）是继承之外的另一大复用机制，它表达两种关系：
**has-a**（有一个——应用域）和 **is-implemented-in-terms-of**
（以某物实现——实现域）。与 public 继承（is-a，item32）三足鼎立：
**选错关系建模工具，是继承滥用的总根源**。

← 上一条 [item37 别重定义缺省参数](./item37-绝不重新定义继承而来的缺省参数值.md)；
下一条 [item39 慎用 private 继承](./item39-明智而审慎地使用private继承.md)。

---

## 1. 三种关系三种工具（选型总表）

| 真实关系 | 建模工具 | 例子 |
|---|---|---|
| **is-a**（行为可替换，LSP） | public 继承 | `TickPrice is-a Price`（语义兼容才成立） |
| **has-a**（有一个，应用域概念） | 复合 | `Order has-a Symbol`、`Book has-a Levels` |
| **is-implemented-in-terms-of**（以某物实现） | 复合（或 item39 私有继承） | `Stack 以 vector 实现`、`Set 以 list 实现` |

**判别口诀**：能说"Derived 就是 Base 的一种，Base 能干的它都能干" → public 继承；
只能说"它内部用到了 Base 的能力" → 复合。

## 2. 教科书反例复习：Stack 与 vector

```cpp
// ❌ public 继承：vector 的全部接口承诺给了用户
class Stack : public std::vector<int> {};   // stack[3]、insert 都能用——栈不变量裸奔

// ✅ 复合：vector 沦为实现细节，接口自己定
class Stack {
    std::vector<int> impl_;
public:
    void push(int x) { impl_.push_back(x); }
    void pop()         { impl_.pop_back(); }
    int& top()         { return impl_.back(); }
};
```

复合的收益：① 接口**只暴露你想给的**（封装完整）；② 实现对用户不可见——
换成 `deque` 用户无感；③ 没有 is-a 谎言（item32 的合同不用硬签）。

## 3. has-a vs is-implemented-in-terms-of（同构不同域）

两者语法一样（都是成员变量），区别在**问题域**：

```cpp
// has-a（应用域：对象之间的概念关系）
class Order {
    Symbol symbol_;       // 订单"有一个"合约——业务概念，天然如此
    Qty qty_;
};

// is-implemented-in-terms-of（实现域：用别人的零件造自己的语义）
class SymbolTable {
    std::unordered_map<std::string, Instrument> impl_;   // 用户不需要知道 map
public:
    const Instrument* find(std::string_view sym) const;
};
```

区分的意义：**has-a 的成员通常是类型语义的一部分**（会出现在接口里，
如 `order.symbol()`）；**实现型成员是纯细节**（永远不该出现在接口——
`SymbolTable` 返回 `map::iterator` 就是泄漏实现，item28 的句柄问题）。

## 4. 复合的优先性：什么时候连"像 is-a"也要选复合

item32 说 public 继承要过 LSP 检验——**检验不过就退到复合**，没有中间态：
- "想复用代码但守不住 Base 的全部语义" → 复合
- "想在中间加一层控制（校验/缓存/同步）" → 复合（适配器/装饰器模式的家）
- "想将来能换实现" → 复合 + 接口（策略/桥接）

继承是**编译期的强耦合**（类型、布局、接口全锁死），复合是**运行期的松耦合**
（指向/持有，可替换）——拿不准时选松的，几乎没有反例。

## HFT 关联

- **订单簿的组成**就是 has-a 的活教材：`Book has-a array<Level>`（档位存储）+
  `Book has-a IndexMap`（id→档位索引）——两个成员各司其职，
  谁也不是谁的"一种"（→ item29 的索引一致性案例）
- **撮合引擎以"订单簿 + 风控器 + 网关"实现**：引擎 is-implemented-in-terms-of
  这三个零件——零件接口化（interface class，item31）后，
  回测换"回放网关"、实盘换"实盘网关"，引擎本体不动
- 复合的内存布局红利：成员是**内联存储**（无指针间接）——
  热路径的 has-a 直接嵌（`Order` 内嵌 `Price`），冷路径的实现隔离才用指针（pImpl）
  ——"复合"与"pImpl"的选择本质是 item31 的热/冷边界

## 代码自测

**题目 1：** `class Car : public Engine` 为什么错？`class Car { Engine engine_; }` 对在哪？

<details>
<summary>参考答案</summary>

车**不是**引擎的一种——"Car 能干的事 Engine 都能干"明显不成立（引擎不会刹车），
is-a 检验（item32）直接失败。这是典型的**把"零件"误当"种类"**。
复合版正确：Car **has-an** Engine——引擎是车的部件，
车通过成员使用引擎的能力（`engine_.start()`），接口完全由 Car 自己定义：
用户看到的是 `car.start()`，看不到引擎的油门/点火细节。
将来换混动引擎，Car 的接口一行不改。

</details>

**题目 2：** has-a 和 is-implemented-in-terms-of 语法上都是成员变量，区分它们的实际意义是什么？

<details>
<summary>参考答案</summary>

语法相同，**设计意图**不同，意图决定接口形态：
① has-a（应用域）：成员是**对象概念的一部分**——会合法出现在接口里
（`order.symbol()` 返回 Symbol 没问题，因为"订单有合约"是业务事实）；
② 实现型（实现域）：成员是**纯粹的秘密**——出现在接口就是泄漏
（`SymbolTable` 返回 `map::iterator` 把"我用 unordered_map"告诉全世界，
换实现时接口全崩，item28）。
review 时的判别问题：这个成员是"业务概念"还是"实现手段"？
后者露头到接口 = 设计事故。

</details>

**题目 3：** 为什么"拿不准选继承还是复合时选复合"几乎没有反例？

<details>
<summary>参考答案</summary>

两个方向的错误成本不对称：
① **该继承却选了复合**：损失只是多写几行转发函数（`void f() { impl_.f(); }`）——
廉价、可逆（哪天确认 is-a 了再改继承，接口不变）；
② **该复合却选了继承**：public 接口被基类绑架（item32 的合同强签）、
is-a 谎言随代码扩散（用户开始依赖基类接口），**剥离成本极高**——
要改的不只是你的类，还有所有已经依赖"D 就是 B"的用户代码。
松耦合 → 强耦合容易，强耦合 → 松耦合难：默认站在松的一边。

</details>
