# Item 12：把重写函数声明为 override

> 第 3 章 移步现代 C++ · Item 12 · 上一节：[Item 11 =default](item11-default.md)

## 为什么要学这个（先建立直觉）

C 没有继承和虚函数，但 C 程序员一定理解"函数指针表"——那正是 vtable 的手工版本：

```c
// C 的"多态"模拟
struct Strategy {
    void (*on_tick)(void* self, const Tick* t);
    void (*on_order)(void* self, const Order* o);
};
```

C++ 的虚函数就是这个模式的语言级支持。但 C++ 有一个 C 没有的坑：**你以为重写了基类虚函数，实际可能没有**。基类参数是 `int`，你写成了 `double`；基类带 `const`，你漏了——这两种情况在 C++11 之前编译器**一声不吭**，代码能编译、能链接、能运行，只是运行时多态不生效。

关键在于：这类 bug **只在基类虚函数提供了默认实现时才会静默**。如果基类是纯虚函数，漏实现会让派生类仍是抽象类，一实例化就编译报错。真正危险的是那些"有默认实现、改了也不会崩、只是行为不对"的虚函数——`override` 正是为它们发明的：它让编译器**检查**是否真的重写了，检查不通过直接编译报错。

---

## 这节讲什么

1. 不加 `override` 时虚函数"静默失效"的现场，以及它为什么只在基类有默认实现时才静默；
2. 加 `override` 后的真实报错原文，逐条对照四种签名不匹配（参数类型 / `const` / 引用限定符 / 基类非虚）；
3. `final`：既挡住重写，又给编译器去虚化（devirtualization）的机会；
4. 引用限定符 `&` / `&&`——`override` 检查里最容易忽略的一项。

---

## 现场：不加 override 时虚函数静默失效

clang 23.1.0，`-std=c++17 -Wall`，完整可编译：

```cpp
#include <iostream>

class Base {
public:
    virtual void on_tick(int x) { std::cout << "Base::on_tick(int) " << x << "\n"; }
    virtual ~Base() = default;
};

class Derived : public Base {
public:
    // 没有 override：参数写成 double，签名不匹配，但编译通过
    virtual void on_tick(double x) { std::cout << "Derived::on_tick(double) " << x << "\n"; }
};

int main() {
    Derived d;
    Base* p = &d;
    p->on_tick(42);      // 期望 Derived，实际 Base
    d.on_tick(3.14);     // 直接调派生类对象 → Derived::on_tick(double)
    return 0;
}
```

真实输出：

```
Base::on_tick(int) 42
Derived::on_tick(double) 3.14
```

第一行的 `Base::on_tick(int)` 就是事故本身——通过基类指针分发，居然落回了基类。`Derived::on_tick(double)` 并没有消失，它成了 `Derived` 自己的新虚函数，只是不在 `Base::on_tick` 的 vtable 槽位上。于是代码"看起来对了、编译过了、单测直接调派生类对象也能过"，只有走多态分发时才暴露。

编译器确实给了一条告警（`-Wall` 下）：

```
warning: 'Derived::on_tick' hides overloaded virtual function [-Woverloaded-virtual]
note: hidden overloaded virtual function 'Base::on_tick' declared here:
      type mismatch at 1st parameter ('int' vs 'double')
```

但告警**不会拦住你**，`override` 会。

---

## 加上 override：把运行时 bug 提前到编译期

```cpp
class Derived : public Base {
public:
    void on_tick(double x) override { std::cout << "Derived::on_tick(double) " << x << "\n"; }
};
```

本机实测 clang 23.1.0 报错原文：

```
error: non-virtual member function marked 'override' hides virtual member function
   11 |     void on_tick(double x) override { ... }
      |                            ^
note: hidden overloaded virtual function 'Base::on_tick' declared here:
      type mismatch at 1st parameter ('int' vs 'double')
```

| 版本 | 编译 | 运行 | 排查成本 |
|------|------|------|---------|
| 不加 `override` | 通过（仅一条告警） | 多态静默失效 | 高：需线上定位"为什么没生效" |
| 加 `override` | **报错** | — | 低：编译器直接写出 `int` vs `double` |

---

## 四种签名不匹配（逐一实测）

| # | 不匹配类型 | `override` 报错关键信息（clang 23.1.0 实测） |
|---|-----------|------------------------------------------|
| ① | 参数类型 | `type mismatch at 1st parameter ('int' vs 'double')` |
| ② | 缺 `const` | `different qualifiers ('const' vs unqualified)` |
| ③ | 引用限定符 | `hides virtual member function` / `cannot overload a member function without a ref-qualifier with a member function with ref-qualifier '&'` |
| ④ | 基类非虚 | `error: only virtual member functions can be marked 'override'` |

②③ 的实测代码：`virtual void process() const;` 配 `void process() override;`（缺 `const`）、`virtual void h() &;` 配 `void h() && override;`（`&` vs `&&`）。

不做 `override` 的对照组（②）：编译通过，程序输出 `Base::process() const`——多态失效。① 最容易踩的变体是 `int` vs `int64_t`、`size_t` vs `uint32_t`：64 位 Linux 上位宽不同，但很多人凭直觉以为"反正都是整数"。

---

## final：override 的搭档

`override` 说"我是重写"，`final` 说"到此为止"。二者正交，可同时写：

```cpp
class Base    { public: virtual void f() { } };
class Derived : public Base { public: void f() override final { } };
class Deep    : public Derived { public: void f() override { } };   // 编译失败
```

实测报错：

```
error: declaration of 'f' overrides a 'final' function
note: overridden virtual function is here
```

`final` 修饰类时：`class Base final { };` → 继承尝试报 `error: base 'Base' is marked 'final'`。

---

## 引用限定符 & 和 &&

成员函数可在参数表后加 `&`（只在左值对象上调）或 `&&`（只在右值对象上调），它和 `const` 一样**属于函数签名**，因此也在 `override` 检查范围内：

```cpp
class Quote {
    double px_ = 0.0;
public:
    explicit Quote(double p) : px_(p) {}
    double& px() &  { return px_; }            // 左值对象：返回引用，还能接着用
    double  px() && { return std::move(px_); }  // 右值对象：可以放心搬走内部数据
};
```

实测输出：

```
lvalue object:     px() &  -> 100.5
rvalue object:     px() && -> 200.25     // Quote(200.25).px()
explicit move:     px() && -> 100.5      // std::move(q).px()
```

---

## 完整可运行例子

HFT 里典型的事件分发点：纯虚的 `on_tick`（漏了直接编译失败）＋ 有默认实现的 `on_reset`（漏了会静默）：

```cpp
#include <iostream>
#include <memory>
#include <vector>

class Handler {
public:
    virtual void on_tick(int px) = 0;      // 纯虚：子类必须重写
    virtual void on_reset() {}             // 有默认实现：写错签名编译器不拦
    virtual ~Handler() = default;
};

class Logger : public Handler {
public:
    void on_tick(int px) override { std::cout << "  Logger::on_tick " << px << "\n"; }
    void on_reset() override { std::cout << "  Logger::on_reset\n"; }
};

class Aggregator : public Handler {
public:
    void on_tick(int px) override { sum += px; }
    void on_reset() override { sum = 0; }
    int sum = 0;
};

int main() {
    std::vector<std::unique_ptr<Handler>> hs;
    hs.push_back(std::make_unique<Logger>());
    hs.push_back(std::make_unique<Aggregator>());
    std::cout << "dispatch on_tick:\n";
    for (auto& h : hs) h->on_tick(100);
    std::cout << "dispatch on_reset:\n";
    for (auto& h : hs) h->on_reset();
    std::cout << "agg->sum after reset = "
              << static_cast<Aggregator*>(hs[1].get())->sum << "\n";
    return 0;
}
```

clang 23.1.0 实测输出：

```
dispatch on_tick:
  Logger::on_tick 100
dispatch on_reset:
  Logger::on_reset
agg->sum after reset = 0
```

把 `Aggregator::on_reset` 的 `override` 去掉并写错签名（比如 `on_reset(int)`），代码依旧能编译——只是 `reset` 永远不会被调用，累计值一直涨。**这就是 `override` 存在的全部理由。**

---

## 常见错误（新手踩坑）

1. **只在纯虚函数上加 `override`**。纯虚函数漏实现时派生类变抽象类，编译器本来就会拦（实测 `error: variable type 'Impl' is an abstract class`）。真正的风险在有默认实现的虚函数上。
2. **把 `-Woverloaded-virtual` 当保险**。它只是告警，且管不到"基类函数压根不是 virtual"的情况——那种写法不构成 hiding，一条告警都不会有。
3. **以为在派生类写 `virtual` 就够了**。`virtual` 在派生类里是冗余的（重写自动为虚），不提供任何检查；`override` 才检查。
4. **改基类签名后靠全局替换**。全局替换能改掉名字，改不掉"语义上这里不该改"。加了 `override` 后编译器会把所有受影响点一次性列出来，这才是安全重构的正确姿势。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 多态 | 函数指针表（手动） | 虚函数（自动） | C++ 语言级支持 |
| 重写检查 | 手动对照（容易出错） | `override` 编译器检查 | 防签名不匹配 |
| 阻止重写 | 不适用 | `final` | 设计封闭类 + 去虚化 |
| 值类别重载 | 不存在 | `&` / `&&` 引用限定符 | 区分"还要用"和"马上死" |

**一句话总结：** C 程序员学 C++ 面向对象时，`override` 是第一条该养成的习惯——它同时检查派生类的签名和基类的 `virtual`，两头都堵。不加它，重写错误就是编译期不可发现的运行时静默失败。

---

## HFT 关联

- **策略基类**：`class Strategy { virtual void on_tick(const Tick&) = 0; };` 子类加 `override` 后签名写错编译期暴露，避免"策略不生效"——这类 bug 在回测里常表现为"收益为 0"而不是"崩溃"，极难察觉。
- **事件处理器**：行情网关回调签名写错（如缺 `const`），不加 `override` 会导致回调不被调用。在 UDP 组播场景下等同于**静默丢包**，无任何告警。
- **`final` 优化**：`final` 让编译器去掉虚调用的间接跳转（devirtualization），省掉一次 vtable 查找和一次潜在的 i-cache miss——HFT 热路径可以受益。

---

## 自测题

**1. 不加 `override` 时签名写错，代码能编译吗？运行时会发生什么？**
<details><summary>答案</summary>
能编译（clang 23.1.0 下仅一条 `-Woverloaded-virtual` 告警）。派生类函数不覆盖基类虚函数，而是成为一个**新的**虚函数，占新的 vtable 槽位。通过基类指针调用时执行基类版本，表现为"改了子类代码却没生效"。实测输出第一行是 `Base::on_tick(int) 42`——事故现场。
</details>

**2. 加上 `override` 后四类签名不匹配，clang 23.1.0 的报错关键信息分别是什么？**
<details><summary>答案</summary>

| 类型 | 报错关键信息 |
|------|-------------|
| ① 参数类型 | `hides virtual member function` + `type mismatch at 1st parameter ('int' vs 'double')` |
| ② 缺 `const` | `hides virtual member function` + `different qualifiers ('const' vs unqualified)` |
| ③ 引用限定符 | `hides virtual member function`；无限定符版另报 `cannot overload a member function without a ref-qualifier with a member function with ref-qualifier '&'` |
| ④ 基类非虚 | `error: only virtual member functions can be marked 'override'` |

④ 这条最有价值：它把"基类的设计意图"也纳入检查——你以为那是虚函数，但基类作者并没打算让你重写；去掉 `override` 后连告警都不会有。
</details>

**3. 基类是 `virtual void process() const`，派生类写成 `void process() override` 会怎样？去掉 `override` 呢？**
<details><summary>答案</summary>
带 `override` 编译失败：`error: non-virtual member function marked 'override' hides virtual member function` / `note: ... different qualifiers ('const' vs unqualified')`。去掉 `override` 则编译通过（带 `-Woverloaded-virtual` 告警），但通过基类指针调用执行的是 `Base::process() const`——多态失效。`const` 是签名的一部分，漏写等于写了另一个函数。
</details>

**4. 引用限定符 `&` / `&&` 的用途是什么？为什么它会影响 `override` 匹配？**
<details><summary>答案</summary>
用途是按调用对象的值类别重载：左值对象（还要继续用）走 `&` 版本，通常返回引用；右值对象（马上销毁）走 `&&` 版本，可以放心把内部资源搬走。实测 `Quote(200.25).px()` 与 `std::move(q).px()` 都走 `&&` 版本。因为 `&` / `&&` 和 `const` 一样属于函数签名，重写要求签名完全匹配，所以基类 `void h() &` 与派生类 `void h() &&`（或无限定符的 `void h()`）不构成重写。
</details>

**5. `override` 和 `final` 能同时用吗？`final` 除了挡重写还有什么作用？**
<details><summary>答案</summary>
能，写作 `void f() override final;`。二者正交：`override` = "我重写了基类虚函数"（编译期校验签名）；`final` = "到此为止，派生类不能再重写"（也可修饰类：`class D final : public B`，此时报 `error: base 'Base' is marked 'final'`）。额外作用：给编译器提供去虚化依据，静态绑定后省掉一次 vtable 间接跳转。
</details>

**6. 为什么"纯虚函数漏实现"反而不是 `override` 的主要战场？**
<details><summary>答案</summary>
因为纯虚函数漏实现（或签名写错导致没覆盖到）时，派生类仍是抽象类，一实例化编译器立刻报硬错误（实测 `error: variable type 'Impl' is an abstract class` + `note: unimplemented pure virtual method 'on_tick' in 'Impl'`），跑不掉。真正的风险在基类提供了默认实现的虚函数上：签名写错后代码完全合法，只是默认实现被悄悄沿用，往往要到生产环境才被发现。
</details>

---

## 参考与延伸

- 下一节：[Item 13 const_iterator](item13-const-iterator.md)
- 回到：[第 3 章 移步现代 C++](README.md)
