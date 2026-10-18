# Item 28：理解引用折叠

> 第 5 章 右值引用、移动语义与完美转发 · Item 28 · 上一节：[Item 27 替代方案](item27-alternatives.md) · 下一节：[Item 29 移动不存在/廉价](item29-move-not-exist.md)

## 为什么要学这个（先建立直觉）

C++ 有条硬性规则：**你不能声明"引用的引用"**。`int& && r = x;` 直接编译失败，实测报错 `error: 'r' declared as a reference to a reference`。

但模板和 `auto` 推导会**间接**造出引用的引用：万能引用接左值时 `T` 被推为 `Widget&`，形参类型 `T&&` 展开就是 `Widget& &&`。语言必须回答"这算什么"，答案就是**引用折叠**（reference collapsing）。这条规则是第 5 章的地基：万能引用能绑左值靠它（[Item 24](item24-universal-vs-rvalue.md)），`std::forward` 能还原值类别靠它，`decltype(auto)` 的行为也靠它。不懂折叠，第 5 章其余内容全是死记硬背；懂了，它们全是这一条规则的推论。

---

## 这节讲什么

1. 四条折叠规则（`static_assert` 全表实测）与 const 为何不参与；
2. 折叠发生的四个语境——也是"引用的引用"唯一合法的四个地方；
3. `auto&&`、`typedef/using`、`decltype` 三个语境的实测结果；
4. `std::forward` 如何用折叠实现（给出 libc++ 同构实现并实测行为）；
5. 折叠与类型推导的关系：`T&&` 为什么会把 `T` 推成 `T&`。

全部论断附本机实测（clang 23.1.0，arm64-apple-darwin，libc++，`-std=c++17 -Wall`）。

---

## 四条规则（实测验证）

| 组合 | 折叠结果 |
|------|----------|
| `&` + `&` | `&` |
| `&` + `&&` | `&` |
| `&&` + `&` | `&` |
| `&&` + `&&` | `&&` |

**口诀：左值引用会传染——组合里出现一个 `&`，结果就是 `&`；只有两个 `&&` 才保持 `&&`。**

直接写 `int& &&` 非法，用 `using` 别名间接组合就合法（clang 23.1.0 实测，四组断言全部通过）：

```cpp
using Lref = int&;
using Rref = int&&;

using A = Lref&;     // &  + &
using B = Lref&&;    // &  + &&
using C = Rref&;     // && + &
using D = Rref&&;    // && + &&

static_assert(std::is_same_v<A, int&>,  "& + &   -> &");
static_assert(std::is_same_v<B, int&>,  "& + &&  -> &");
static_assert(std::is_same_v<C, int&>,  "&& + &  -> &");
static_assert(std::is_same_v<D, int&&>, "&& + && -> &&");
```

对照：直接声明的报错原文（`-std=c++17 -Wall`）。

```
error: 'r' declared as a reference to a reference
    int main() { int x = 0; int& && r = x; (void)r; }
                                 ^
```

`const` 不参与折叠——折叠只解决引用性，cv 限定各自独立保留。实测 `using CLref = const int&; using E = CLref&&;` 得 `const int&`（const 原样留下，引用性被压成 `&`）。同理 **`&& + &` 的顺序不影响结果**：口诀是"有 `&` 则 `&`"。

---

## 折叠发生的四个语境

C++ 里"引用的引用"只能**间接**出现在这四个地方，折叠也只在这四个地方发生：

| 语境 | 例子 | 折叠前 → 后 |
|---|---|---|
| 模板类型推导 | `template<class T> void f(T&&)`，左值实参使 `T=T&` | `T& &&` → `T&` |
| `auto` 推导 | `auto&& r = 左值;` | `auto=T&` → `T& &&` → `T&` |
| `typedef` / `using` 别名 | `using A = Lref&&;` | `int& &&` → `int&` |
| `decltype` | `decltype(rx)&&`（`decltype(rx)` 是 `int&`） | `int& &&` → `int&` |

其他任何地方出现"引用的引用"都是编译错误。折叠是**纯编译期的类型计算**，生成的代码里没有任何"折叠"动作。

---

## 用折叠解释万能引用

[Item 24](item24-universal-vs-rvalue.md) 说"万能引用接左值时 `T` 推为 `T&`"，当时没解释最后一步——现在补上。实测（用 `if constexpr` 把 `T` 和 `decltype(x)` 打印出来）：

```cpp
template<class T> void f(T&& x) { /* 打印 name<T>() 与 name<decltype(x)>() */ }
int lv = 1; const int clv = 2;
f(lv);   f(clv);   f(3);
```

本机实测 clang 23.1.0 输出：

```
f(左值):          T=int&          decltype(x)=int&
f(const 左值):    T=const int&    decltype(x)=const int&
f(右值):          T=int           decltype(x)=int&&
```

推导链条：`f(左值)` → `T` 被推为 `int&` → 形参类型 `T&&` 展开为 `int& &&` → 折叠成 `int&` → 能绑左值 ✓。`f(右值)` → `T=int` → 形参 `int&&` → 无需折叠 → 绑右值 ✓。**同一个 `T&&` 形参靠"推导让 T 带引用" + "折叠把双引用压回单引用"两步实现左右值通吃。**

### auto&& 是同一套规则

`auto` 推导就是模板推导（[Item 2](../ch01-deducing-types/item02-auto-type-deduction.md)），所以 `auto&&` 也是万能引用，也走折叠。实测：

```
  auto&& <- 左值      : int&
  auto&& <- const 左值: const int&
  auto&& <- 右值      : int&&
```

因此 `for (auto&& x : container)` 是容器遍历的万能写法：左值元素（多数容器）绑成 `T&`，代理迭代器返回的右值（如 `vector<bool>` 的 `reference`）绑成 `T&&`，两种都能正确绑定。

### decltype 语境

`decltype` 产出的是类型（含引用性），再往上叠引用同样触发折叠。实测：

```
  decltype(rx)&  = int&      // rx 声明为 int&
  decltype(rx)&& = int&      // 有 & 则 &，叠 && 也救不回来
```

---

## std::forward 如何用折叠实现

libc++ 里 `std::forward` 就长这样（取自本机 `/Users/a0000/micromamba/envs/cdev/bin/../include/c++/v1/__utility/forward.h`，去掉属性宏）：

```cpp
template <class T>
T&& forward(remove_reference_t<T>& t) noexcept {
    return static_cast<T&&>(t);          // ★ 折叠发生在这里
}

template <class T>
T&& forward(remove_reference_t<T>&& t) noexcept {
    static_assert(!is_lvalue_reference<T>::value, "cannot forward an rvalue as an lvalue");
    return static_cast<T&&>(t);
}
```

把这段改成自己的 `my_forward`（把 `remove_reference_t` 写全就是 C++11 版）后实测，行为与 `std::forward` 完全一致：

```
fwd(s)              :  左值重载
fwd(std::string())  :  右值重载
fwdWrongL(s)        :  左值重载     ← forward<std::string&>，恒定左值
fwdWrongL(rvalue)   :  左值重载     ← 右值被压成左值（写错 T 的代价）
fwdWrongR(s)        :  右值重载     ← 左值被抬成右值（写错 T 的代价）
```

`static_cast<T&&>` 按 `T` 分情况：

- **左值转发**：`T = Widget&` → `static_cast<Widget& &&>` → 折叠为 `Widget&` → 返回类型也是 `T&&` = `Widget&` → 传出左值；
- **右值转发**：`T = Widget` → `static_cast<Widget&&>` → 返回类型 `Widget&&` → 传出右值。

`forward` 的全部"智能"就是把 `T` 的引用性塞进 `static_cast<T&&>`，然后**让折叠规则干活**。它有 `static_assert` 兜底 `forward<T&>(右值)` 这种明显写错的调用。理解了折叠，`forward` 就没有任何神秘之处——它是个一行流。

### 为什么必须显式写 `std::forward<T>`

因为形参类型 `remove_reference_t<T>&` 里 `T` 出现在**不可推导语境**（non-deduced context）：编译器没法从实参类型反推出 `T`。实测漏写会直接编不过：

```
error: no matching function for call to 'forward'
    auto&& r = std::forward(s);
                            ^~~~~~~~~
note: candidate template ignored: couldn't infer template argument '_Tp'
```

对照 `std::move`——它的形参是 `T&&`（可推导），返回 `__libcpp_remove_reference_t<T>&&`，**不依赖折叠**：先把 `T` 的引用性用 `remove_reference_t` 抹平，再无脑加 `&&`。所以 `move(x)` 可以不写模板实参，而 `forward<T>(x)` 必须写。这个"必须手写类型"的设计正是 Item 25 反复强调"别把 forward 写成 move"的根因——写错一个字母就改变值类别。

---

## 完整可运行例子

```cpp
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

using Lref = int&;
using Rref = int&&;
static_assert(std::is_same_v<Lref&,  int&>,  "& + &   -> &");
static_assert(std::is_same_v<Lref&&, int&>,  "& + &&  -> &");
static_assert(std::is_same_v<Rref&,  int&>,  "&& + &  -> &");
static_assert(std::is_same_v<Rref&&, int&&>, "&& + && -> &&");

template<class T> constexpr const char* nm() {
    using X = T;
    if constexpr (std::is_same_v<X, std::string&>)       return "string&";
    else if constexpr (std::is_same_v<X, std::string>)   return "string";
    else if constexpr (std::is_same_v<X, std::string&&>) return "string&&";
    else return "?";
}

// 手写 forward（与 libc++ 同构）
template<class T>
T&& fwd_impl(std::remove_reference_t<T>& x) noexcept { return static_cast<T&&>(x); }

void sink(std::string&  ) { std::cout << "  -> 左值重载\n"; }
void sink(std::string&& ) { std::cout << "  -> 右值重载\n"; }

template<class T> void relay(T&& x) {
    std::cout << "  T=" << nm<T>() << " / decltype(x)=" << nm<decltype(x)>()
              << " / 形参折叠后=" << nm<decltype(x)>();
    sink(fwd_impl<T>(x));
}

int main() {
    std::string s = "tick";
    relay(s);                 // 左值
    relay(std::string("px")); // 右值

    std::cout << "auto&& <-\n";
    auto&& p1 = s;                    std::cout << "  左值: " << nm<decltype(p1)>() << "\n";
    auto&& p2 = std::string("qty");   std::cout << "  右值: " << nm<decltype(p2)>() << "\n";
}
```

本机实测 clang 23.1.0（`-std=c++17 -Wall`）输出：

```
  T=string& / decltype(x)=string& / 形参折叠后=string&  -> 左值重载
  T=string / decltype(x)=string&& / 形参折叠后=string&&  -> 右值重载
auto&& <-
  左值: string&
  右值: string&&
```

---

## 常见错误（新手踩坑)

1. **试图直接写 `int& &`**。语法错误（`declared as a reference to a reference`）——折叠只在四个间接语境发生。
2. **把折叠当成运行时行为**。它是纯编译期类型计算，生成的机器码里没有任何"折叠"。
3. **以为 `&& + &` 顺序影响结果**。不影响——口诀是"有 `&` 则 `&"`。
4. **以为 const 也参与折叠**。不参与：`const &` + `&&` 折叠为 `const &`，const 原样保留。
5. **`forward` 漏写模板实参**。实测 `couldn't infer template argument '_Tp'`——`T` 在 `remove_reference_t<T>&` 里是不可推导语境。而 `move` 可以省略实参，两者别混。
6. **手写 forward 时把 `static_cast<T&&>` 写成 `static_cast<T&>`**。前者依赖折叠保留值类别，后者无条件给左值——转发形同虚设。

---

## HFT 关联

- **读框架代码的钥匙**：muduo / 策略框架里 `template<class T> void setCallback(T&& f)`、`template<class... Args> void emplace(Args&&... args)` 满地都是——理解折叠才能推断每个调用点 `T` 实际推成了什么、转发后会不会意外拷一份。
- **完美转发的正确性闭环**：`T&&` 接参（Item 24）+ 折叠（本节）+ `std::forward<T>` 传出（Item 25）三者构成完整链条，缺一环就是"看似转发了实际多拷一次"的热路径隐形开销。
- **`auto&&` 遍历省心**：`for (auto&& t : ring)` 同时吃左值元素和代理引用，不需要为不同容器写两个版本的循环。

---

## 自测题

**1. 引用折叠的四条规则是什么？用一句口诀概括。**

<details><summary>答案</summary>
`&+&→&`、`&+&&→&`、`&&+&→&`、`&&+&&→&&`。口诀：**左值引用会传染**——组合里出现一个 `&`，结果就是 `&`；只有两个 `&&` 才保持 `&&`。已用 `using` 别名造出四组引用叠引用，`static_assert` 全部通过。
</details>

**2. 为什么 `int& &&` 不能直接写，但 `using A = Lref&&;`（`Lref = int&`）合法？**
<details><summary>答案</summary>
语言规则禁止**直接声明**"引用的引用"，实测报错 `error: 'r' declared as a reference to a reference`。但折叠规则存在的意义就是处理**间接**产生的引用叠引用——别名展开、模板推导、`decltype` 这些语境里类型是"算"出来的，语言不禁止计算过程产生叠引用，而是规定用折叠把它压回单引用。
</details>

**3. 折叠发生在哪四个语境？**
<details><summary>答案</summary>
模板类型推导、`auto` 推导、`typedef`/`using` 别名、`decltype`。这四个也是 C++ 中仅有的能间接产生"引用的引用"的地方。
</details>

**4. 用折叠规则解释：`f(w)`（`w` 是 `Widget` 左值）调用 `template<class T> void f(T&&)` 时形参的最终类型。**
<details><summary>答案</summary>
`T` 推为 `Widget&`，形参类型 `T&&` 展开为 `Widget& &&`，按规则 `& + && → &`，最终形参是 `Widget&`——所以能绑左值。实测印证：`T=string& / decltype(x)=string&`。
</details>

**5. `std::forward<T>(x)` 在 `T = Widget&` 和 `T = Widget` 时各返回什么？折叠起了什么作用？**
<details><summary>答案</summary>
`T = Widget&`：返回类型 `T&&` = `Widget& &&` 折叠为 `Widget&`，传出左值；`T = Widget`：返回 `Widget&&`，传出右值。折叠是 forward 的全部机制——`static_cast<T&&>` 把 `T` 的引用性带进类型，折叠自动给出正确值类别，不需要任何运行时分支。
</details>

**6. 为什么 `std::forward` 必须显式写模板实参，而 `std::move` 不用？**
<details><summary>答案</summary>
forward 的形参是 `remove_reference_t<T>&`，`T` 处在**不可推导语境**，编译器反推不出 `T`——实测漏写报 `couldn't infer template argument '_Tp'`。move 的形参是 `T&&`（可推导），返回 `__libcpp_remove_reference_t<T>&&`，先用 `remove_reference_t` 抹平引用性再无脑加 `&&`，**不依赖折叠**，所以可以不写实参。
</details>

**7. `const Widget& &&` 折叠成什么？const 参与折叠吗？**
<details><summary>答案</summary>
`const Widget&`。const 不参与折叠——折叠只解决引用性（`&`/`&&`），cv 限定各自独立保留。实测 `using CLref = const int&; using E = CLref&&;` 得 `const int&`。
</details>

---

## 参考与延伸

- 上一节：[Item 27 替代方案](item27-alternatives.md)
- 下一节：[Item 29 移动不存在/廉价](item29-move-not-exist.md)
- 相关：[Item 24 区分万能引用与右值引用](item24-universal-vs-rvalue.md)（折叠的第一现场）
- 相关：[Item 33 泛型 lambda](../ch06-lambda-expressions/item33-generic-lambda.md)（`decltype(x)` 拿的就是折叠后的形参类型）
- 回到：[第 5 章 右值引用、移动语义与完美转发](README.md)
