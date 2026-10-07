# 条款 35：考虑 virtual 函数以外的其他选择

## 本节讲什么

**Consider alternatives to virtual functions.** 虚函数是多态的经典答案，但不是唯一答案。
本条款给出四个替代方案及其取舍：**NVI 惯用法、函数指针/策略对象、
std::function 策略、CRTP 编译期多态**——现代 C++（尤其性能敏感场景）里，
后两个常常比虚函数更合适。本机 g++ 13.3 实测 NVI 与策略对象。

← 上一条 [item34 接口 vs 实现继承](./item34-区分接口继承和实现继承.md)；
下一条 [item36 别重定义非虚函数](./item36-绝不重新定义继承而来的non-virtual函数.md)。

---

## 1. 替代一：NVI（Non-Virtual Interface）惯用法（本机实测）

把"虚"藏进 private，public 只暴露非虚外壳——**模板方法模式的现代形态**：

```cpp
class Strategy {
public:
    void on_tick() {              // 非虚外壳：框架控制前置/后置逻辑
        pre_check();              // 风控前置（策略碰不到也绕不开）
        do_on_tick();             // 定制点
        post_log();               // 审计后置
    }
    virtual ~Strategy() = default;
private:
    virtual void do_on_tick() { std::cout << "默认策略\n"; }   // 私有虚：唯一定制点
    void pre_check() {} void post_log() {}
};
class MyStrat : public Strategy {
    void do_on_tick() override { std::cout << "我的策略\n"; }   // 本机实测输出
};
```

**收益**（相对"public 虚函数裸奔"）：
① 骨架纪律（前置/后置）无法被派生类覆盖绕开；② 定制点收窄到明确位置；
③ 外壳里可以加锁/日志/计时，派生类零感知。
（→ item34 形态四；M3 并发里 lock 包裹的惯用法同理念。）

## 2. 替代二：策略对象（std::function / 函数指针注入，本机实测）

行为不挂在类型上，**作为数据注入**——连继承都不要了：

```cpp
class Strategy2 {
    std::function<void()> impl_;
public:
    explicit Strategy2(std::function<void()> f) : impl_(std::move(f)) {}
    void on_tick() { impl_(); }
};
Strategy2 s([] { std::cout << "lambda 策略\n"; });   // 本机实测输出
s.on_tick();
```

| 维度 | 虚函数 | 策略对象 |
|---|---|---|
| 行为绑定时机 | 编译期（类型决定） | **运行期**（构造时注入） |
| 行为能否运行时更换 | 不能 | **能**（换 impl_ 即可） |
| 类型数量 | 每种策略一个类 | **一个类 + N 个 lambda** |
| 状态捕获 | 成员变量 | lambda 捕获 |
| 开销 | vtable 跳转 | 同（std::function 一次间接）+ 可能小对象分配 |

适合：策略数量多且轻（几十个规则各几行）、需要运行时拼装
（回测参数扫同一骨架换不同 lambda）。

## 3. 替代三：CRTP——编译期多态（零间接）

```cpp
template <typename Derived>
class StrategyBase {
public:
    void on_tick() {
        pre_check();
        static_cast<Derived*>(this)->do_on_tick();   // 编译期解析：无 vtable、可内联
        post_log();
    }
private:
    void pre_check() {} void post_log() {}
};
class MyStrat : public StrategyBase<MyStrat> {
public:
    void do_on_tick() { /* 热路径：直接调用，编译器可见可内联 */ }
};
```

**收益**：零 vtable、零间接调用——"多态"在编译期完成（→ ch16 模板、M1）；
**代价**：① 不同类型的策略**不能放同一个基类指针容器**
（`StrategyBase<A>` 和 `StrategyBase<B>` 是不同类型）——异构集合仍需虚函数或
type erasure；② 每个实例化一份代码（二进制膨胀）。

## 4. 四方案选型表

| 需求 | 选谁 |
|---|---|
| 框架要守住骨架纪律（前置/后置） | **NVI** |
| 行为要运行时换/策略多到不值得建类 | **std::function 策略对象** |
| 热路径 + 类型集合编译期已知 | **CRTP**（配 variant 可补异构集合） |
| 类型集合开放 + 需要异构指针容器 | **经典虚函数**（它是为此而生） |

**组合用法**（真实框架）：引擎对策略是虚函数接口（异构容器），
策略内部的热路径分发用 CRTP（编译期），可插拔规则用 std::function（运行期）——
三个层次各用各的，不混。

## HFT 关联

- **热路径多态的第一反应应该是 CRTP 或查表，不是虚函数**：
  行情分发的 msg_type → handler 用成员指针表（→ 19.3）；
  策略骨架用 CRTP（零间接可内联）
- 虚函数在**冷边界**仍无可替代：策略注册表（`vector<unique_ptr<Strategy>>`）、
  交易所网关接口（多后端 + mock）——异构集合是它的主场
- `std::function` 的类型擦除有隐藏分配（小对象优化外的 lambda 捕获）——
  热路径注入策略时先算捕获大小（→ M1 Item，function 的 SBO 边界）

## 代码自测

**题目 1：** NVI 相比"public 虚函数 + 文档约定"，强在哪？

<details>
<summary>参考答案</summary>

public 虚函数下，派生类 override 时可以**完全替换**行为——
框架约定的"先风控后处理"靠文档和自觉，新人一个 override 就绕开了。
NVI 把骨架锁进**非虚** public 外壳（派生类无法覆盖非虚函数，
item36 还会告诉你重定义它有多糟），定制点收进 **private 虚函数**——
派生类能实现定制点，但**无法决定它何时被调用、前后发生什么**。
纪律从"约定"升级为"类型系统强制"（本机实测：pre_check/post_log 必然执行）。

</details>

**题目 2：** `std::function` 策略对象相比虚函数继承，什么时候是更好选择？

<details>
<summary>参考答案</summary>

① 策略**数量多且每个很轻**——几十个规则各几行代码，建几十个类是官僚主义；
② 需要**运行时更换/拼装**行为（回测参数扫描：同一骨架换 100 组参数
就是 100 个 lambda，虚函数得写 100 个类或带参构造）；
③ 行为天然是**无层次**的（不属于任何类型家族）。
虚函数继承的主场是：策略有共同状态/生命周期、需要异构指针容器、
行为集合需要被类型系统区分（`Strategy*` vs `RiskRule*` 不同接口）。

</details>

**题目 3：** CRTP 实现的多态为什么不能放同一个基类指针容器？想要"异构集合 + 零间接"怎么折中？

<details>
<summary>参考答案</summary>

`StrategyBase<MyStrat>` 和 `StrategyBase<YourStrat>` 是**两个毫不相干的类型**
（模板按参数各实例化一份）——没有共同基类，`vector<StrategyBase*>` 这种类型
根本写不出来。CRTP 的多态是编译期的，容器需要运行期的统一类型。
折中：**std::variant**——`variant<StratA, StratB, StratC>` 装异构策略，
`std::visit` 分发（编译期展开、无 vtable、还可能内联）；
类型集合真正开放（运行期注册新策略）时，才回到虚函数 + type erasure。
"编译期已知集合 → variant/CRTP；运行期开放集合 → 虚函数"是清晰的边界。

</details>
