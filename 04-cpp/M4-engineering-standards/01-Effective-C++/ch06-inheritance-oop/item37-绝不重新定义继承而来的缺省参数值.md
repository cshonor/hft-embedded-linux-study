# 条款 37：绝不重新定义继承而来的缺省参数值

## 本节讲什么

**Never redefine a function's inherited default parameter value.** 虚函数动态绑定、
缺省参数**静态绑定**——两者混用时，同一个调用会出现"派生类的函数体 +
基类的默认值"的诡异组合。本机 g++ 13.3 实测复现了这个教科书级陷阱。

← 上一条 [item36 别重定义非虚函数](./item36-绝不重新定义继承而来的non-virtual函数.md)；
下一条 [item38 复合塑模 has-a](./item38-通过复合塑模出has-a或以某物实现.md)。

---

## 1. 现象：函数体动态、参数静态（本机实测）

```cpp
struct Base {
    virtual void log(const char* level = "INFO") { std::cout << "Base::log " << level << "\n"; }
};
struct Derived : Base {
    void log(const char* level = "DEBUG") override { std::cout << "Derived::log " << level << "\n"; }
};
Derived d;
Base& b = d;
d.log();    // Derived::log DEBUG       ← 静态类型 Derived：默认值 DEBUG
b.log();    // Derived::log INFO (!!)   ← 函数体是 Derived 的（动态绑定），
            //                           默认值却是 Base 的（静态绑定）
```

**本机实测输出**：

```
Derived::log DEBUG
Derived::log INFO      ← 这一行就是陷阱本身：Derived 的实现拿到了 Base 的默认值
```

**机制**：虚函数的**调用目标**看对象动态类型（vtable），
但缺省参数在**编译期**按调用表达式的**静态类型**填充——
`b.log()` 在编译期就被改写为 `b.log("INFO")`，运行期才经 vtable 找到
`Derived::log` 执行。两套绑定机制各管一半，组合出谁都不想要的语义。

## 2. 为什么语言要设计成这样（理解才能记住）

缺省参数若也动态绑定，每次虚调用都要**运行期查默认值**——
vtable 里得存默认参数表，调用开销显著增加。
C++ 选择了效率：默认值是**编译期常量替换**，零成本——
代价就是本条要守的纪律。这是"效率优先于直觉"的经典 C++ 决策。

## 3. 正确姿势

**方案一（推荐）：NVI——默认值放在非虚外壳上，虚函数不带默认值**

```cpp
class Base {
public:
    void log(const char* level = "INFO") {   // 非虚外壳：默认值只出现一次
        do_log(level);                        // 转发给定制点
    }
    virtual ~Base() = default;
private:
    virtual void do_log(const char*) const {} // 虚函数**没有**缺省参数
};
```

非虚外壳静态绑定（默认值只有一个权威定义），虚函数传**显式参数**——
静态/动态各归其位，陷阱从结构上消失（item35 NVI 的又一收益）。

**方案二：保持默认值全继承链一致**——派生类 override 时**照抄**基类的默认值。
能编译但脆：基类改默认值，所有派生类要同步改，漏一个就是本条陷阱。

## 4. 快速自查

```cpp
// 危险信号：override 的函数声明里出现 '='
void on_order(const Order& o, bool urgent = false) override;   // ← 检查基类默认值是否一致
```

团队约定：**虚函数的缺省参数只许出现在最顶层基类，或全走 NVI**。

## HFT 关联

- 策略回调的缺省参数（`on_tick(t, from_replay = false)`）若框架用
  `Strategy&` 基类引用调用——派生类改默认值 = 实盘/回测行为分裂，
  且**不会触发任何警告**；NVI 化（`run(t)` 外壳 + `do_on_tick(t, flags)` 显式参数）
  是引擎框架的标准答案
- 协议解码虚函数的默认值（`decode(buf, strict = true)`）同理：
  回放路径 strict 语义必须唯一权威——一处默认值定义，全链照抄或 NVI

## 代码自测

**题目 1：** `b.log()` 为什么输出 "Derived::log INFO" 而不是 "Derived::log DEBUG"？

<details>
<summary>参考答案</summary>

两套绑定各管一半：**函数体**动态绑定——`b` 指向 Derived 对象，vtable 查到
`Derived::log` 执行；**缺省参数**静态绑定——`b` 的静态类型是 `Base&`，
编译期就把 `b.log()` 改写为 `b.log("INFO")`（本机实测复现）。
Derived 里写的默认值 "DEBUG" 只在**静态类型为 Derived** 的调用（`d.log()`）中生效。
想避免：NVI（默认值放非虚外壳）或全链默认值一致。

</details>

**题目 2：** 语言为什么不把缺省参数也做成动态绑定？

<details>
<summary>参考答案</summary>

效率。动态绑定默认值意味着：vtable 里除了函数指针还要存**默认参数表**，
每次虚调用都要运行期查表取默认值——而虚调用恰恰是 C++ 里
"每纳秒都计较"的机制。静态绑定的默认值是编译期常量替换，零运行期成本。
C++ 的传统决策：给你机制（NVI）绕开陷阱，但不为直觉牺牲效率。
理解这一点，"静态绑定默认值"就从"怪癖"变成"可推理的设计"。

</details>

**题目 3：** 用 NVI 重写题目 1 的 Base/Derived，让 `b.log()` 语义唯一。

<details>
<summary>参考答案</summary>

```cpp
class Base {
public:
    void log(const char* level = "INFO") { do_log(level); }   // 非虚：默认值唯一权威
    virtual ~Base() = default;
private:
    virtual void do_log(const char* lv) const { std::cout << "Base::log " << lv << "\n"; }
};
class Derived : public Base {
    void do_log(const char* lv) const override { std::cout << "Derived::log " << lv << "\n"; }
};
```

`b.log()`：非虚外壳静态绑定 → `do_log("INFO")` → 虚分发到 Derived 实现——
输出 "Derived::log INFO"，**任何调用路径语义一致**。
默认值只存在一份（外壳），派生类想改默认行为只能 override 实现，
碰不到参数默认值——陷阱从结构上消除。

</details>
