# 条款 21：按需把函数声明为虚函数，不要盲目虚函数

## 本节讲什么

**Declare functions virtual only when needed.** 本条与 item20 一体两面：
item20 管"绑定方式的成本"，本条管"**哪个函数**该进虚函数表"——
类的每个虚函数都是一张长期账单（vtable 条目 + 内联阻断 + ABI 冻结）。
给类的接口做"虚函数预算"：只给**真正的定制点**付账。

← 上一条 [item20 绑定方式](./item20-按需选用静态绑定动态绑定（虚函数），不要无脑虚函数增加开销.md)；
下一条 [item22 接口 vs 实现继承](./item22-区分接口继承和实现继承，很多继承设计错误根源就在这里.md)。

---

## 1. 虚函数的三张账单（每个 virtual 都在付）

| 账单 | 内容 |
|---|---|
| 调用成本 | 每次调用间接跳转（→ item20 成本解剖） |
| 优化成本 | 该函数**永不可内联**（除非去虚拟化成功）——小 getter 变虚 = 白送的优化丢了 |
| ABI 成本 | 虚函数表**顺序**进入 ABI——加/删/重排虚函数 = 二进制不兼容（已编译的用户代码 vtable 偏移全错） |

第三张最隐蔽：`class Widget { virtual void a(); virtual void b(); }` 发布后，
想加 `virtual void c()` 只能加**末尾**；想删 `a()` 会移动 b 的 vtable 槽位——
所有已编译的调用方错位崩溃。虚函数接口的演化自由度远低于非虚接口。

## 2. 定制点识别法（哪些才配 virtual）

```text
真定制点：不同派生类的行为**必须不同**，且调用方需要统一接口
     → 策略的 on_tick、网关的 send、序列化的 serialize
假定制点（不该虚的常见错误）：
     ① "给未来留余地"的虚函数——YAGNI（→ item20 决策框架）
     ② 实现细节的 helper——它们应该是 private 非虚（或 NVI 的私有虚，item35）
     ③ 全类统一的工具函数（id()/name()）——非虚（→ Effective item36 的重定义陷阱）
```

**NVI 的关系**（→ item35/Effective item34）：把"骨架"留非虚（public），
定制点收窄为**私有虚函数**——虚函数预算从"接口上每个函数"减到
"明确的定制点一两个"。

## 3. 析构函数的特例（永远虚？）

**基类的析构必须虚**（经基类指针 delete 时调对析构）——这是唯一
"默认该虚"的函数。但注意推论：
- 不当基类的类**不要**虚析构（白付 vptr 税）——"每个类都 virtual ~T()" 是
  Java 转 C++ 的经典过度设计
- 纯接口（无数据）用 `virtual ~I() = default;`——一个虚槽换 delete 安全，值

## HFT 关联

- 策略基类的虚函数清单评审：`on_tick/on_order/on_timer` 是真定制点（虚），
  `id()/name()/is_ready()` 全类统一（非虚）——虚函数预算精确到个位数
- ABI 冻结在**行情解码库**是真实约束：协议升级加虚函数只能加末尾，
  这就是为什么解码接口倾向**非虚 + 模板/查表**（→ 19.3 成员指针表：
  扩展加表项不动 vtable）
- 热路径类的虚函数预算 = 0：订单簿/价格档/行情消息全 POD（→ 19.5）——
  "这个类有 vptr 吗"该是 review 的第一问

## 代码自测

**题目 1：** 为什么说虚函数表的顺序是 ABI 的一部分？

<details>
<summary>参考答案</summary>

虚调用编译后是 `vptr->vtable[N]()`——**槽位号 N 在编译期烙进调用方二进制**。
你在类中间插入/删除虚函数，后续所有虚函数的槽位号**平移**：
旧编译的调用方按旧槽位号调，调到的是**错误的函数**（不报错，行为错乱——
比崩溃更可怕）。
所以发布库的虚接口演化规则：新增虚函数只能**加在末尾**；
删/重排 = 全量重编译。这就是为什么成熟框架把扩展点设计成
"非虚 + 回调注册"（表驱动扩展）而不是"加虚函数"——vtable 是 ABI 的化石层。

</details>

**题目 2：** "每个类都写 virtual ~T() = default"错在哪？

<details>
<summary>参考答案</summary>

它给**不当基类**的类强加了 vptr 税：对象 +8B（小对象尺寸翻倍）、
vtable 一张、delete 路径多一次间接——而收益（经基类指针安全 delete）
在"这个类根本不被继承"时**永远用不到**。
规则反过来才对：**设计为基类**（会被经基类指针 delete）→ 虚析构；
具体类/值类型/叶子类 → 非虚析构（连 vptr 都不该有）。
辅助工具：C++11 起给叶子类标 `final`——既向读者声明"不做基类"，
又帮编译器去虚拟化（→ item20 final 的收益链）。

</details>

**题目 3：** 用 NVI 把"虚函数预算"降到最低，给一个策略基类的接口设计。

<details>
<summary>参考答案</summary>

```cpp
class Strategy {
public:
    // 全非虚外壳：骨架纪律锁死（→ item35 NVI 实测）
    void on_tick(const Tick& t) { if (armed_) do_on_tick(t); }
    void on_order(const ExecRpt& r) { audit(r); do_on_order(r); }
    int id() const { return id_; }              // 非虚：全类统一（Effective item36）
    bool is_ready() const { return armed_; }    // 非虚
    virtual ~Strategy() = default;              // 唯一的 public 虚（delete 安全）
private:
    virtual void do_on_tick(const Tick&) = 0;   // 定制点仅两个，且私有（不可绕开骨架）
    virtual void do_on_order(const ExecRpt&) {}
    int id_ = 0; bool armed_ = false;
};
```

虚函数预算：**2 个私有定制点 + 1 个析构**——
接口上其余十几个函数全部静态绑定（可内联、无 ABI 演化负担）。
派生类能实现定制点，但骨架（armed 检查/audit）永远绕不开——
虚函数的最小化与纪律的最大化同时达成。

</details>
