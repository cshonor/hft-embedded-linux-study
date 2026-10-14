# 条款 28：不要返回指向对象内部成员的句柄

## 本节讲什么

**Avoid returning "handles" to object internals.** "句柄"（handle）=
指针、引用、迭代器——任何能让用户**绕过公共接口直接操纵内部**的东西。
返回它们会带来三类破坏：**封装作废**（比 public 成员更糟）、**const 被架空**、
**句柄悬垂**（对象死了句柄还活着）。这条与 item22（private）、item23（最小接口）
是同一道防线的三个面。

← 上一条 [item27 减少类型转型](./item27-尽量减少类型转型（cast）.md)；
下一条 [item29 异常安全](./item29-编写异常安全的代码.md)。

---

## 1. 三类破坏

### ① 非 const 引用返回成员 = 成员变量实质 public

```cpp
class Order {
    Qty qty_;
public:
    Qty& qty() { return qty_; }        // ❌ qty() 成了"可写的后门"——
};                                      //    用户 o.qty() = -100; 不变量裸奔
```

这违反了 item22 的精神却披着"用了函数接口"的外衣——
**private 成员 + 可写引用 getter = 最差的组合**（比直接 public 更糟：
public 至少诚实，这里用户还以为有封装）。

### ② const 成员函数返回非 const 句柄 = const 语义架空

```cpp
class Book {
    std::vector<Level> levels_;
public:
    const std::vector<Level>& levels() const { return levels_; }   // ✅ const 引用，只读
    std::vector<Level>& levels() { return levels_; }               // ❌ const 对象也能调非 const 重载？
};

void audit(const Book& b) {
    // b.levels().push_back(...);      // 若只提供非 const 版本：编译通过，audit 能改 b！
}
```

`const Book&` 的 promise 是"我不改你"——返回非 const 句柄让这个 promise 形同虚设。

### ③ 句柄悬垂：句柄的寿命 ≤ 对象的寿命

```cpp
const std::string& name(const Order& o) { return o.name_; }   // 返回成员引用

const std::string& bad = name(Order{});    // ❌ 临时 Order 行末析构，bad 悬垂
auto good = name(order);                   // ✅ order 是长寿对象——但改名后 good 跟着变（别名耦合）
```

迭代器版本更隐蔽：`auto it = book.levels().begin();` 之后 `levels_` 任何
扩容/修改都让 `it` 失效——句柄不仅随对象死而悬，还随**对象内部变化**而悬。

## 2. 什么时候返回句柄是合法的（边界）

| 场景 | 合法形态 |
|---|---|
| 只读访问且对象长寿 | **const 引用/指针**（`const std::string& name() const`） |
| 容器元素访问是本职 | `operator[]`/`at`/`begin`——容器的存在就是为了给元素句柄 |
| 调用方需要可写但你要守不变量 | **写函数**（`set_qty(q)` 里校验），不给可写句柄 |
| 性能关键的批量只读 | const 引用 + 文档写明"容器死/改则失效"（`span`/string_view 同理念） |

**判别口诀**：句柄让用户做的事，**公共接口愿意背书吗**？
只读观察 → const 句柄可以；改动内部 → 走写函数，句柄不给。

## 3. 与"输出参数"的关系（item21 的联动）

热路径常用的输出参数模式，本质就是"调用方提供句柄，函数往里写"——
方向反过来（句柄由调用方持有并负责寿命），就避开了本条的所有坑：

```cpp
bool decode(const Buf& b, Tick& out);     // ✅ out 的寿命归调用方，无悬垂问题
// vs
const Tick& decode(const Buf& b);         // ❌ 返回谁内部的引用？静态的？上次调用的？
```

## HFT 关联

- **订单簿给策略层的接口**：只读视图（`best_bid() const -> Price` 按值）+
  写操作走函数（`add/cancel`）——给策略一个 `Level&` 等于让策略绕过撮合引擎的不变量
- `std::string_view`/`std::span<const T>` 是"受控句柄"的现代形态：
  明确"只读 + 寿命随源"语义，协议解析里大量使用（零拷贝字段视图，
  → M5 C++17 ch15 string_view）
- 缓存友好考虑：按值返回小 POD（`Price`/`Qty`，8B 走寄存器）通常**比**
  返回引用还快（省一次间接）——热路径"按值小对象 + 句柄只给大对象"是经验法则

## 代码自测

**题目 1：** `const std::string& name() const { return name_; }` 有哪些潜在问题？

<details>
<summary>参考答案</summary>

① **悬垂**：`name(Order{})`——临时对象行末析构，引用成野引用；
② **别名耦合**：`auto& n = o.name(); o.rename("x");`——n 的内容跟着变，
调用方可能没预期"我存的引用会自己变"；
③ **线程**：另一线程改 name_ 时，持有引用的线程读到撕裂数据（无同步）。
改进：小对象按值返回（`std::string name() const` 对短名 SSO 零分配）；
或明确文档"引用寿命随对象"。

</details>

**题目 2：** 为什么"const 成员函数返回非 const 引用"比"直接 public 成员变量"更糟？

<details>
<summary>参考答案</summary>

public 成员变量至少**诚实**——用户看到 `obj.qty_` 知道这是裸露数据。
而 `const Order& o; o.qty() = x;` 能编译通过时，const 的契约被**静默架空**：
review 代码的人看到 `const Order&` 参数，合理推断函数不会改它——
但实际通过句柄改了。谎言比赤裸更危险：它破坏了团队对 const 语义的整体信任，
让所有 const 标注的审计价值归零。

</details>

**题目 3：** 订单簿要给策略层"只读看盘口"的能力，设计两个方案并比较。

<details>
<summary>参考答案</summary>

方案一：**按值快照**——`TopOfBook top() const { return {bids_[0], asks_[0]}; }`：
小 POD 拷贝走寄存器/SSO，无句柄问题，策略看到的永远是"那一刻"的一致快照；
代价是深度数据拷不起（只能给 top N 档）。
方案二：**只读句柄 + 寿命约定**——`std::span<const Level> bids() const`：
零拷贝看全深，但策略必须接受"簿子一改 span 就失效"（回调内使用有效，
跨回调持有非法）。
热路径实践：同回调内用方案二（零拷贝），跨回调/跨线程用方案一（快照）——
或上 RCU/seqlock 版本化视图（→ M3 并发 ch07）。

</details>
