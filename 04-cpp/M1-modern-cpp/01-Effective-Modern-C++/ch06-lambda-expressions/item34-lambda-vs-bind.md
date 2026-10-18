# Item 34：优先 lambda 而非 std::bind

> 第 6 章 Lambda 表达式 · Item 34 · 上一节：[Item 33 泛型 lambda](item33-generic-lambda.md)

## 为什么要学这个（先建立直觉）

`std::bind` 是 C++11 的产物，解决的是「把函数的部分实参先固定下来，剩下的以后再传」——C 程序员要做同样的事得手写包装函数。2011 年它是个进步，因为那一年 lambda 还不能移动捕获、没有泛型形参，有些活只有 bind 能干。

但 C++14 之后形势翻转：[Item 32](item32-init-capture.md) 的 init capture 补齐了移动捕获，[Item 33](item33-generic-lambda.md) 的泛型 lambda 补齐了延迟参数——bind 的每一项能力 lambda 都有，而且 lambda 可读、可内联、报错友好。bind 剩下的只有一堆坑：占位符 `_1`/`_2` 和实参的映射要靠脑补、实参在 bind 时就求值、重载函数要 `static_cast` 消歧、默认拷贝实参想用引用得包 `std::ref`。

你还会在大量 2011–2014 年的老代码（和老面试题）里见到 bind，所以要能读懂它；但新代码没有理由再写它。这一节用四个实测把 bind 的坑逐个坐实，然后给出结论。

---

## 这节讲什么

1. bind 的**占位符语义**：`_1` 是「调用时再传」的第 1 个实参，可以重排（反直觉）；
2. **求值时机**实测：bind 的实参在 bind 时求值，lambda 的 init capture 同样在创建时求值，lambda 体内才是调用时求值；
3. **重载函数**交给 bind 必须 `static_cast` 消歧（报错原文实测）；
4. bind **默认拷贝实参**，要引用得 `std::ref`（实测对比）；
5. 结论：lambda 全面胜出，bind 仅历史价值。

---

## 占位符语义：_1 是「洞」

`std::bind(f, a, _1)` 的含义：造一个可调用对象，调用它时把第 1 个实参填进 `_1` 这个「洞」，其余位置用 bind 时存的值。本机实测（clang 23.1.0，`-std=c++17 -Wall`）：

```cpp
#include <functional>
#include <iostream>
#include <string>

void report(int pri, const std::string& msg) {
    std::cout << "[" << pri << "] " << msg << "\n";
}

int main() {
    using namespace std::placeholders;
    auto r = std::bind(report, 1, _1);   // pri 固定为 1，msg 留洞
    r("hello");                          // report(1, "hello")

    // 占位符还能重排——调用时的第 2 个实参填进函数的第 1 个形参
    auto swap2 = std::bind(
        [](int a, int b){ std::cout << a << "," << b << "\n"; }, _2, _1);
    swap2(1, 2);                         // 输出 2,1 —— 反直觉！
    return 0;
}
```

实测输出：

```
[1] hello
2,1
```

`swap2(1, 2)` 输出 `2,1`——`_2` 写在前面就把调用方的第 2 个实参放到了函数的第 1 个位置。读 bind 表达式要在脑子里跑一遍「占位符编号 → 调用方实参序号 → 目标函数形参位置」的两级映射，这就是它晦涩的根源。lambda 版本一眼见底：

```cpp
auto swap2 = [](int a, int b) { return std::invoke(目标, b, a); };
// 或者直接：
auto swap2 = [](int a, int b){ std::cout << b << "," << a << "\n"; };
```

---

## 求值时机：bind 时 vs 调用时

bind 的实参（非占位符部分）在 **bind 那一刻**求值并存进 bind 对象；lambda 的 init capture 同样在**创建时**求值；只有 lambda **体内**的表达式才是调用时求值。实测（带打印的 `compute()`）：

```cpp
#include <functional>
#include <iostream>

int compute() {
    std::cout << "compute() 执行\n";
    return 42;
}
void show(const char* tag, int v) { std::cout << tag << ": " << v << "\n"; }

int main() {
    using namespace std::placeholders;
    std::cout << "== bind 版本 ==\n";
    auto f = std::bind(show, "bind", compute());    // compute() 现在就跑
    std::cout << "-- bind 对象已创建 --\n";
    f();

    std::cout << "== lambda 版本 ==\n";
    auto g = [v = compute()] { show("lambda", v); }; // init capture 也是创建时
    std::cout << "-- lambda 已创建 --\n";
    g();

    auto h = [] { show("lambda-延迟", compute()); }; // 体内调用：调用时求值
    std::cout << "-- h 已创建 --\n";
    h();
    return 0;
}
```

本机实测（clang 23.1.0）输出：

```
== bind 版本 ==
compute() 执行
-- bind 对象已创建 --
bind: 42
== lambda 版本 ==
compute() 执行
-- lambda 已创建 --
lambda: 42
-- h 已创建 --
compute() 执行
lambda-延迟: 42
```

bind 和 init capture 的 `compute()` 都在对象创建**之前**打印——两者语义相同；只有 lambda 体内的 `compute()` 延迟到了 `h()` 调用时。所以「bind 在 bind 时求值、lambda 在调用时求值」的流行说法**只对 lambda 体成立**，对 init capture 不成立——这一点 Item 32 的实测也印证了。

---

## 重载函数：bind 必须 static_cast 消歧

重载函数名在表达式里没有唯一类型，bind 推导不出来。本机实测报错原文（clang 23.1.0）：

```cpp
int value(int x) { return x; }
double value(double x) { return x; }
auto f = std::bind(&value, _1);
```

```
error: no matching function for call to 'bind'
note: candidate template ignored: couldn't infer template argument '_Fp'
```

必须手动消歧：

```cpp
auto vi = std::bind(static_cast<int(*)(int)>(&value), _1);
std::cout << vi(5) << "\n";   // 实测输出：50
```

lambda 天然免疫——重载决议发生在 lambda 体内正常的函数调用点，编译器按实参类型自动选：

```cpp
auto vi = [](int x) { return value(x); };   // 自动选中 int 版本，无需 cast
```

---

## 默认拷贝实参：std::ref 的坑

bind 把非占位符实参**按值拷贝**进 bind 对象（准确说是 decay-copy）。想改到原件必须显式 `std::ref`：

```cpp
struct Counter {
    int n = 0;
    void bump() { ++n; }
};

Counter c;
auto b1 = std::bind(&Counter::bump, c);            // 拷了一份 c
auto b2 = std::bind(&Counter::bump, std::ref(c));  // 持有引用
b1();  b1();
std::cout << "b1 两次后 c.n = " << c.n << "\n";
b2();  b2();
std::cout << "b2 两次后 c.n = " << c.n << "\n";
```

本机实测（clang 23.1.0）输出：

```
b1 两次后 c.n = 0（拷贝，没改到原件）
b2 两次后 c.n = 2（std::ref，改到原件）
```

`b1` 改的是 bind 对象内部那份拷贝，原件纹丝不动——忘了 `std::ref` 的 bug 不报错、不警告，静默无效。lambda 的捕获列表把「拷贝还是引用」写在明面上：`[c]` 拷贝、`[&c]` 引用、`[c = std::move(c)]` 移动，没有第三种隐藏规则。

---

## 完整可运行例子

同一任务两种写法对照：固定优先级上报消息。

```cpp
#include <functional>
#include <iostream>
#include <memory>
#include <string>

void report(int pri, const std::string& msg) {
    std::cout << "[" << pri << "] " << msg << "\n";
}

int main() {
    using namespace std::placeholders;

    // bind 版本：pri 固定 1，msg 留洞
    auto b = std::bind(report, 1, _1);
    b("from bind");

    // lambda 版本：一目了然，可内联
    auto l = [](const std::string& msg) { report(1, msg); };
    l("from lambda");

    // lambda 还能做 bind 做不到/很绕的事：移动捕获（Item 32）
    auto pw = std::make_unique<std::string>("owned");
    auto m = [pw = std::move(pw)] { report(9, *pw); };
    m();
    return 0;
}
```

本机实测（clang 23.1.0，`-std=c++17 -Wall`）输出：

```
[1] from bind
[1] from lambda
[9] owned
```

---

## 常见错误（新手踩坑）

1. **占位符重排后读错参数顺序**。`std::bind(f, _2, _1)` 调用 `g(1, 2)` 实际是 `f(2, 1)`（实测输出 `2,1`）。占位符编号指「调用方的第几个实参」，不是「目标函数的第几个形参」。
2. **忘了 `std::ref` 导致静默拷贝**。`std::bind(&Counter::bump, c)` 拷了一份 `c`，调用改了副本，原件不变（实测 `c.n` 仍为 0）。不报错不警告。
3. **重载函数直接交给 bind**。`std::bind(&value, _1)` 报 `couldn't infer template argument '_Fp'`（实测原文），必须 `static_cast<int(*)(int)>(&value)`；lambda 写法 `[](int x){ return value(x); }` 无需消歧。
4. **bind 成员函数忘了传对象**。`std::bind(&Widget::process, _1)` 漏了 `this`，调用签名对不上；正确是 `std::bind(&Widget::process, &w, _1)`。lambda 写 `[&w](auto& d){ w.process(d); }` 不会犯这个错。

---

## HFT 关联

- **热路径回调必须可内联**：lambda 的闭包类型编译期已知，`std::sort(v.begin(), v.end(), [](auto& a, auto& b){ ... })` 比较器直接内联进排序循环；bind/函数指针是间接调用，每 tick 成千上万次比较时差距可观。回测引擎、订单簿遍历全部用 lambda。
- **bind 的隐藏拷贝是延迟地雷**：`std::bind(process, big_config)` 在 bind 时拷贝整个配置对象，还可能是 decay-copy 丢掉 const/引用——热路径上这种「看不见的拷贝」比显式拷贝更难排查。lambda 捕获列表把拷贝/引用/移动写在明面上。
- **老代码迁移**：2015 年前的策略框架里 `bind` 常见，迁移时逐条换成 lambda，顺手把 `std::ref` 语义改成显式 `[&c]`，代码评审时捕获清单一目了然。
- **占位符重排是事故源**：`bind(f, _2, _1)` 这类参数对调写法在 lambda 里是 `[](auto a, auto b){ f(b, a); }`——参数名让意图显式，code review 不用解码 `_N`。

---

## 自测题

**1. `std::bind(report, 1, _1)` 中 `_1` 的含义是什么？`std::bind(f, _2, _1)` 调用 `g(1, 2)` 时实参如何传递？**
<details><summary>答案</summary>
`_1` 是一个「洞」：表示调用 bind 返回的对象时传入的第 1 个实参。`std::bind(report, 1, _1)` 固定了第一个形参 `pri=1`，`msg` 留给调用方。占位符编号指**调用方的实参序号**而非目标函数的形参位置，所以 `std::bind(f, _2, _1)` 被 `g(1, 2)` 调用时执行的是 `f(2, 1)`——本文实测输出 `2,1`，反直觉，是 bind 可读性差的主要原因。
</details>

**2. bind 的非占位符实参与 lambda 的 init capture，求值时机各是什么？「lambda 在调用时求值」的说法哪里不准确？**
<details><summary>答案</summary>
bind 的非占位符实参在 **bind 时**求值并存入 bind 对象；lambda 的 init capture 在 **lambda 创建时**求值——两者时机相同（本文实测：`compute()` 的打印都在「对象已创建」之前）。「lambda 在调用时求值」只对 lambda **体内**的表达式成立，对捕获列表不成立；要延迟到调用时计算，得把表达式写进 lambda 体。
</details>

**3. 为什么重载函数不能直接交给 `std::bind`？怎么处理？lambda 为什么没有这个问题？**
<details><summary>答案</summary>
重载函数名不对应唯一类型，`bind` 的模板形参 `_Fp` 推导失败（clang 实测报 `couldn't infer template argument '_Fp'`）。处理办法是 `static_cast` 消歧：`std::bind(static_cast<int(*)(int)>(&value), _1)`。lambda 里重载决议发生在 lambda 体内的普通调用点，编译器按实参类型自动选重载，`[](int x){ return value(x); }` 无需任何 cast。
</details>

**4. `std::bind(&Counter::bump, c)` 调用后 `c` 会变吗？为什么？**
<details><summary>答案</summary>
不会。bind 对实参做 decay-copy，`c` 被**拷贝**进 bind 对象，`bump()` 改的是那份拷贝（本文实测：调用两次后 `c.n` 仍为 0）。要改原件必须 `std::ref(c)`——实测 `std::ref` 版本两次调用后 `c.n == 2`。lambda 的对应语义是显式的：`[c]` 拷贝、`[&c]` 引用，不会静默选错。
</details>

**5. 既然 lambda 全面胜出，bind 还有什么存在价值？**
<details><summary>答案</summary>
历史价值为主：读懂和维护 C++11 时代的老代码（当时 lambda 没有 init capture 和泛型形参，移动捕获、延迟参数只能靠 bind）；以及极少数「多态函数对象的部分应用」场景 bind 更紧凑。Meyers 的结论是：C++14 起新代码一律 lambda——可读性、可内联性、错误信息、捕获控制全面占优。
</details>

**6. lambda 可内联为什么对 HFT 重要？bind 为什么不行？**
<details><summary>答案</summary>
lambda 的闭包类型在编译期完全已知，调用点能看到函数体，编译器可内联并进一步做常量传播、循环展开；bind 返回的对象通过内部函数指针/类型擦除间接调用目标，编译器通常看不到最终调用目标，无法内联。HFT 热路径每 tick 成千上万次调用（比较器、过滤器、回调分发），每次间接调用的分支预测失败和调用开销会累积成可观延迟。
</details>

---

## 参考与延伸

- 上一节：[Item 33 泛型 lambda](item33-generic-lambda.md)
- 下一章：[第 7 章 并发 API](../ch07-concurrency-api/README.md)
- 回到：[第 6 章 Lambda 表达式](README.md)
