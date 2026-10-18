# Item 33：对 auto&& 形参用 decltype + std::forward（泛型 lambda）

> 第 6 章 Lambda 表达式 · Item 33 · 上一节：[Item 32 初始化捕获](item32-init-capture.md) · 下一节：[Item 34 lambda vs bind](item34-lambda-vs-bind.md)

## 为什么要学这个（先建立直觉）

函数模板的完美转发你已经会了：`template<class T> void f(T&& x) { g(std::forward<T>(x)); }`。C++14 把泛型能力给了 lambda（参数可以写 `auto`），问题随之而来：**lambda 没有你能写的模板参数名 `T`**——`forward` 需要显式模板实参，`forward<???>(x)` 里填什么？

Meyers 的答案：`decltype(x)`。形参 `x` 的声明类型里带着推导出的引用性，`decltype` 原样取出（[Item 3](../ch01-deducing-types/item03-decltype.md) 的规则在这里派上用场），交给 `forward` 正好够用。这是第 1 章（decltype）、第 5 章（万能引用 + forward）两条线在 lambda 里的合流点。

---

## 这节讲什么

1. 泛型 lambda 的真面目：闭包类里的模板 `operator()`（含手写等价类对照）；
2. 为什么完美转发必须配 `std::forward<decltype(x)>(x)`，`std::move` 错在哪（实测）；
3. `decltype(x)` 为什么给出引用类型，`decltype((x))` 为什么会把右值性弄丢；
4. 返回类型推导的规则与冲突报错；
5. C++20 显式模板参数列表 lambda（终于能写出 `T`）；
6. 与 `std::function` 的配合，以及类型擦除的实测代价。

本机环境：clang 23.1.0，arm64-apple-darwin，libc++，默认 `-std=c++17 -Wall`，C++20 段落标 `-std=c++20`。

---

## 泛型 lambda = 闭包类里的模板 operator()

写 `[](auto&& x){...}` 时，编译器生成的闭包类大致是这样（手写等价版实测行为一致）：

```cpp
struct CompilerClosure {
    template<class __T>
    auto operator()(__T&& x) const { return x + x; }
};
```

`__T` 是编译器内部的名字——**你的 lambda 体里够不着它**，这是后面所有麻烦的根源。实测同一个 lambda 被不同类型调用（这本身就是"`operator()` 是模板"的证据）：

```
twice(21)          = 42          // __T = int
twice(2.5)         = 5           // __T = double
twice(std::string) = abab        // __T = std::string
手写闭包 cc(21)     = 42
手写闭包 cc(2.5)    = 5
```

`decltype(x)` 是唯一能合法拿到这个类型的途径。

---

## 必须配 forward<decltype(x)>(x)

```cpp
void func(std::string&  s);   // 左值重载
void func(std::string&& s);   // 右值重载

auto fwd = [](auto&& x){ func(std::forward<decltype(x)>(x)); };
auto bad = [](auto&& x){ func(std::move(x)); };
```

clang 23.1.0 实测：

```
fwd(s)          :  左值重载: tick-long-enough-to-beat-sso-optimization
fwd("临时")      :  右值重载: 临时
bad(s)          :  右值重载: tick-long-enough-to-beat-sso-optimization
```

`auto&&` 是万能引用（有 `auto` 推导，[Item 24](../ch05-rvalue-move-forwarding/item24-universal-vs-rvalue.md)）：左值实参 → `auto` 推为 `T&`（形参折叠成 `T&`）；右值实参 → `auto` 推为 `T`（形参 `T&&`）。`decltype(x)` 取形参的**声明类型**——正是 `forward` 需要的输入。

### decltype(x) 为什么给出引用类型

实测（`nm<T>()` 把类型名打印出来）：

```
probe(左值):
  decltype(x)   = std::string&
  decltype((x)) = std::string&
probe(右值):
  decltype(x)   = std::string&&
  decltype((x)) = std::string&
```

两条结论：

- **`decltype(x)` 走的是 Item 3 的"分支一"**——`x` 是**变量名的 id-expression**，结果是它的**声明类型**，完整保留引用性与 cv：左值实参时 `T&`、右值实参时 `T&&`。
- **`decltype((x))` 是"分支二"**——`(x)` 是**表达式**，左值表达式恒给 `T&`。实测右值实参时它把 `std::string&&` 压成了 `std::string&`，**右值性丢失**，forward 退化成左值转发。**转发语境永远写 `decltype(x)`，一个括号都不能多加。**

### 反面教材：std::move 的代价（本机实测纠正）

`std::move` 是**无条件**转右值，左值实参也被强行移动。笔记里流传多年的说法——"SSO 短字符串 move 后内容还在"——**在本机不成立**：

```
std::string short_s = "tick";                                   // SSO：数据存在对象里
std::string long_s  = "AAPL 20260710 C 230.00 x100 @1.2345";    // 堆分配
auto takes = [](auto&& x){ std::string owned = std::move(x); ... };

  取走: tick
  move 后 short_s = '' (size=0)
  取走: AAPL 20260710 C 230.00 x100 @1.2345
  move 后 long_s  = '' (size=0)
```

本机 libc++ 上**短串和长串移动后都被清空**。标准只保证源对象处于"有效但未指定"状态——它现在恰好是空串，换实现可以直接改成别的有效值。**结论不变但理由要更正：不能用"看起来没被掏空"当安全证据，得靠写法正确。**对照 `forward` 的版本：左值实参时被**拷贝**进 `owned`，源对象内容完好（实测 `lv = '左值内容-足够长-...'` 保留）。

---

## 返回类型推导

泛型 lambda 的返回类型按 `auto` 规则推导（每次实例化独立推导）。两条分支返回类型不一致时实测报错：

```cpp
auto pick = [](auto x){ if (x > 0) return x; else return 0.0; };   // int / double 冲突
std::cout << pick(1) << "\n";
```

```
error: 'auto' in return type deduced as 'double' here but deduced as 'int' in earlier return statement
note: in instantiation of function template specialization 'main()::(lambda)::operator()<int>' requested here
```

需要稳定分支类型时，显式写尾置返回类型：`[](auto x) -> double { ... }`。

---

## C++20：终于能写 T 了

C++20 允许 lambda 带显式模板参数列表（clang 23.1.0 + `-std=c++20` 实测）：

```cpp
auto f = []<typename T>(T&& x){ func(std::forward<T>(x)); };   // T 有名字了

auto probe = []<typename T>(T&& x){ std::cout << __PRETTY_FUNCTION__ << "\n"; };
auto conv  = []<typename T>(T x){ return static_cast<double>(x); };
conv.template operator()<long>(7);                             // 甚至可以显式指定实参
auto sum2  = []<std::integral... Ts>(Ts... xs){ return (xs + ...); };  // 变参 + 约束
```

实测输出：

```
  auto main()::(lambda)::operator()(T &&) const [T = std::string &]
  auto main()::(lambda)::operator()(T &&) const [T = std::string]
  auto main()::(lambda)::operator()(T &&) const [T = int]
conv.template operator()<long>(7): 7
sum2(1,2,3) = 6
```

注意 `__PRETTY_FUNCTION__` 里的 `(lambda)::operator()`——**这直接证实了"闭包类的 operator() 是模板"**。另外 `-std=c++17` 下这段会给出 `warning: explicit template parameter list for lambdas is a C++20 extension`（作为扩展仍能编译），不要误以为已经在 C++17 合法。

---

## 与 std::function 的配合：类型擦除只冻结一种签名

`std::function` 做的是**类型擦除**，它必须先把"要调用什么签名"固化下来：

```cpp
auto twice = [](auto&& x){ return x + x; };
std::function<int(int)> fi = twice;    // 冻结成 int(int)，此时才实例化 operator()<int>
fi(21);                                // OK：42
```

擦除之后泛型就消失了——实测再用别的类型调用：

```
error: no matching function for call to object of type 'std::function<int (int)>'
    std::cout << f2(std::string("x")) << "\n";
                 ^~
note: candidate function not viable: no known conversion from 'std::string' to 'int' for 1st argument
```

所以 **`std::function` 存泛型 lambda 时，你选的那个签名就是它的全部能力**。要保留泛型请用 `auto` 变量或模板参数传递闭包。

### 性能实测（`-O2`，取 5 轮最小值）

```
泛型 lambda 直调       : 0.31 ~ 0.39 ns/次
再包一层 lambda        : 0.31 ~ 0.35 ns/次
std::function 类型擦除 : 0.94 ns/次
类型擦除 / 直调 = 2.4x ~ 3.0x
```

`auto` 变量持有的 lambda 一路上都会被内联展开（实测"再包一层"没有任何额外开销）；`std::function` 每次调用要走一次间接跳转（通过擦除后的 invoker 指针），实测约多付 0.5-0.6 ns/次。绝对数字很小，但它决定了：**回调注册可以用 `std::function`，热路径上逐条调用的地方要用模板传闭包。**

---

## 完整可运行例子

```cpp
#include <functional>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

void func(std::string&  s) { std::cout << "  左值重载: " << s << "\n"; }
void func(std::string&& s) { std::cout << "  右值重载: " << s << "\n"; }

template<class T> const char* nm() {
    if constexpr (std::is_same_v<T, std::string&>)       return "std::string&";
    else if constexpr (std::is_same_v<T, std::string>)   return "std::string";
    else if constexpr (std::is_same_v<T, std::string&&>) return "std::string&&";
    else return "?";
}

int main() {
    auto fwd = [](auto&& x){ func(std::forward<decltype(x)>(x)); };
    auto bad = [](auto&& x){ func(std::move(x)); };

    std::string s = "tick-long-enough-to-beat-sso-optimization";
    std::cout << "fwd(s)     :"; fwd(s);
    std::cout << "fwd(右值)  :"; fwd(std::string("新生"));
    std::cout << "bad(s)     :"; bad(s);

    auto probe = [](auto&& x){
        std::cout << "  decltype(x)   = " << nm<decltype(x)>()
                  << " / decltype((x)) = " << nm<decltype((x))>() << "\n";
    };
    std::cout << "probe(左值):\n"; probe(s);
    std::cout << "probe(右值):\n"; probe(std::string("y"));

    auto twice = [](auto&& x){ return x + x; };
    std::cout << "twice(21) = " << twice(21) << " / twice(std::string(ab)) = " << twice(std::string("ab")) << "\n";
    std::function<int(int)> fi = twice;
    std::cout << "std::function<int(int)>::fi(21) = " << fi(21) << "\n";
}
```

本机实测 clang 23.1.0（`-std=c++17 -Wall`）输出：

```
fwd(s)     :  左值重载: tick-long-enough-to-beat-sso-optimization
fwd(右值)  :  右值重载: 新生
bad(s)     :  右值重载: tick-long-enough-to-beat-sso-optimization
probe(左值):
  decltype(x)   = std::string& / decltype((x)) = std::string&
probe(右值):
  decltype(x)   = std::string&& / decltype((x)) = std::string&
twice(21) = 42 / twice(std::string(ab)) = abab
std::function<int(int)>::fi(21) = 42
```

---

## 常见错误（新手踩坑）

1. **泛型 lambda 里用 `std::move`**。实测 `bad(s)` 走右值重载——左值实参被允许掏空。本机实测纠正：SSO 短串移动后**同样被清空**（`size=0`），"短串看不出问题"的说法在这套 libc++ 上不成立。
2. **试图 `forward<auto>(x)`**——`auto` 不是类型，不能当模板实参。`decltype(x)` 是唯一通道（C++20 前）。
3. **`decltype((x))` 多加一对括号**。实测右值实参时它把 `std::string&&` 压成 `std::string&`，右值性丢失、转发退化。永远写 `decltype(x)`。
4. **以为 `std::function` 能保住泛型**。它只冻结一个签名，实测用别的类型调用报 `no matching function for call to object of type 'std::function<int (int)>'`。
5. **返回类型推导冲突**。两条 `return` 类型不一致时报 `deduced as 'double' here but deduced as 'int' in earlier return statement`——显式写 `-> T`。
6. **在 `-std=c++17` 下用 C++20 的模板参数列表写法**。能编（作为扩展），但会出 `-Wc++20-extensions` 警告——不要把带警告的代码当已标准化。
7. **普通 lambda 过度泛化**。参数类型明明确定（只接 `const Tick&`），就别写 `auto&&`——泛型接口的接受面越大，误用面越大。

---

## HFT 关联

- **回调适配层**：策略引擎里把用户 lambda 包一层再接进事件循环（记日志、计时、异常吞掉）——包装层必须完美转发，否则 tick 对象在每一跳被多拷一次。`[](auto&& tick){ timer.scoped([&]{ user_cb(std::forward<decltype(tick)>(tick)); }); }` 是标准件。
- **`std::function` 别用在每 tick**：实测类型擦除多付 0.5-0.6 ns/次且有间接跳转。回调注册/存储用它没问题，逐条调用的热路径要用模板传闭包（`template<class F> void onTick(F&& cb)`），让 `operator()` 内联展开。
- **decltype 全家桶的汇合点**：Item 3 的 decltype 规则 + 第 5 章的万能引用 / 引用折叠 / forward，在泛型 lambda 这一个语法点上全部用上——它是检验前面内容是否真懂的试金石。

---

## 自测题

**1. 泛型 lambda 的 `auto&&` 参数如何配合 `std::forward` 实现完美转发？为什么不能写 `forward<T>`？**
<details><summary>答案</summary>
`auto&&` 是万能引用：左值实参使 `auto` 推为 `T&`（形参折叠成 `T&`），右值实参推为 `T`（形参 `T&&`）。`decltype(x)` 取出形参的声明类型——左值时 `T&`、右值时 `T&&`——交给 `std::forward<decltype(x)>(x)` 即可还原值类别（实测左值走左值重载、右值走右值重载）。不能写 `forward<T>` 是因为那个模板参数是编译器合成的内部名，源码里没有可写的 `T`。
</details>
**2. 为什么说"泛型 lambda = 闭包类里的模板 operator()”？有什么实测证据？**
<details><summary>答案</summary>
手写 `struct CompilerClosure { template<class __T> auto operator()(__T&& x) const { return x + x; }; };` 与 `[](auto&& x){ return x + x; }` 行为一致（实测 `cc(21)=42`、`cc(2.5)=5`）。更直接的证据：同一个 lambda 能被 `int`/`double`/`std::string` 分别调用；`__PRETTY_FUNCTION__` 打印出 `main()::(lambda)::operator()(T &&) const [T = std::string &]`——明写了 `operator()` 是模板。
</details>
**3. `decltype(x)` 在 `auto&&` 形参上拿到什么？左右值实参时分别是什么？为什么？**
<details><summary>答案</summary>
拿到形参 `x` 的声明类型，完整保留引用性与 cv：左值实参时 `std::string&`，右值实参时 `std::string&&`（实测）。因为 `x` 作为变量名的 id-expression，走的是 Item 3 的"分支一"——给声明类型；它不像 `auto` 那样剥引用。
</details>
**4. `decltype((x))` 有什么问题？实测证据是什么？**
<details><summary>答案</summary>
`(x)` 是表达式，走 Item 3 的"分支二"——左值表达式恒给 `T&`。实测右值实参时 `decltype(x)=std::string&&` 而 `decltype((x))=std::string&`：右值性被压掉，forward 退化成左值转发。**永远写 `decltype(x)`，不多不少一个括号。**
</details>
**5. 用 `std::move` 代替 `forward<decltype(x)>` 会发生什么？本机对 SSO 的实测结论是什么？**
<details><summary>答案</summary>
左值实参也被强制转右值：实测 `bad(s)`（`s` 是左值）走了右值重载。关于"看起来没事"的细节需要更正——本机 libc++ 上**短串（SSO）和长串移动后 `size` 都变成 0**（实测 `move 后 short_s = '' (size=0)`）。标准只保证"有效但未指定"，现在是空串、换个实现可以是任何有效值。所以"没看到破坏"绝不是证据，唯一可靠的是写法正确。
</details>
**6. C++20 给泛型 lambda 加了什么新写法？在 C++17 下会怎样？**
<details><summary>答案</summary>
显式模板参数列表：`[]<typename T>(T&& x){ func(std::forward<T>(x)); }`——模板参数终于有了可写的名字，还能配合约束（`[]<std::integral... Ts>(Ts... xs)`）与显式指定实参（`conv.template operator()<long>(7)`）。`-std=c++17` 下能编但给出 `warning: explicit template parameter list for lambdas is a C++20 extension`，不要当成已标准化。
</details>
**7. 泛型 lambda 存进 `std::function` 会丢什么？性能代价实测多少？**
<details><summary>答案</summary>
丢泛型本身：`std::function<int(int)> f = twice;` 只冻结 `int(int)` 这一个实例化，再用 `std::string` 调用报 `no matching function for call to object of type 'std::function<int (int)>'`。性能上（`-O2`，5 轮最小值）泛型 lambda 直调约 0.31-0.39 ns/次、`std::function` 约 0.94 ns/次，即约 2.4-3.0 倍、多付约 0.5-0.6 ns/次；而"再包一层 lambda"没有任何额外开销（完全内联）。
</details>

---

## 参考与延伸

- 上一节：[Item 32 初始化捕获](item32-init-capture.md)
- 下一节：[Item 34 lambda vs bind](item34-lambda-vs-bind.md)
- 相关：[第 5 章 Item 28 引用折叠](../ch05-rvalue-move-forwarding/item28-reference-collapsing.md)（`decltype(x)` 拿到的就是折叠后的形参类型）
- 相关：[第 1 章 Item 3 decltype](../ch01-deducing-types/item03-decltype.md)（`decltype(x)` 与 `decltype((x))` 的分支规则）
- 回到：[第 6 章 Lambda 表达式](README.md)
