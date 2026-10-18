# Item 28：理解引用折叠

> 第 5 章 · Item 28 · 上一节：[Item 27 替代方案](item27-alternatives.md)

## 为什么要学这个（先建立直觉）

C++ 有条硬性规则：**你不能声明"引用的引用"**——`int& & r = x;` 直接编译失败。但模板和 `auto` 推导会**间接**造出引用的引用：万能引用接左值时 `T` 被推为 `Widget&`，形参类型 `T&&` 就变成了 `Widget& &&`——一个"引用的引用"。语言必须回答"这算什么"，答案就是引用折叠（reference collapsing）。

这条规则是整个第 5 章的地基：万能引用能绑左值靠它（[Item 24](item24-universal-vs-rvalue.md)），`std::forward` 能还原值类别靠它，`decltype(auto)` 的行为也靠它。不懂折叠，第 5 章的其他内容全是死记硬背；懂了折叠，它们全是一条规则的推论。

---

## 这节讲什么

1. 四条折叠规则（static_assert 全表实测）；
2. 折叠发生的四个语境——也是"引用的引用"唯一合法的四个地方；
3. 用折叠解释万能引用为什么能绑左值；
4. `std::forward` 如何用折叠还原值类别。

---

## 四条规则（实测验证）

| 组合 | 折叠结果 |
|------|----------|
| `&` + `&` | `&` |
| `&` + `&&` | `&` |
| `&&` + `&` | `&` |
| `&&` + `&&` | `&&` |

**口诀：左值引用会传染——组合里出现一个 `&`，结果就是 `&`；只有两个 `&&` 才保持 `&&`。**

clang 23.1.0 实测（用别名间接造出"引用的引用"，全部断言通过）：

```cpp
#include <type_traits>
using std::is_same_v;

using Lref = int&;
using Rref = int&&;

using A = Lref&;     // &  + &
using B = Lref&&;    // &  + &&
using C = Rref&;     // && + &
using D = Rref&&;    // && + &&

static_assert(is_same_v<A, int&>);
static_assert(is_same_v<B, int&>);
static_assert(is_same_v<C, int&>);
static_assert(is_same_v<D, int&&>);   // 唯一活下来的一对

// decltype 语境同理：decltype(rx) 给 int&，再加 & 触发折叠
int x = 0; int& rx = x;
static_assert(is_same_v<decltype(rx)&, int&>);
```

注意写法上的取巧：`int& &&` 直接写是非法的，但通过 `using` 别名间接组合（`Lref&&`）就合法——折叠规则就是为这种"间接产生"准备的。

---

## 折叠发生的四个语境

C++ 里"引用的引用"只能间接出现在这四个地方，折叠也只在这四个地方发生：

1. **模板类型推导**：万能引用 `T&&` 接左值时 `T` 推为 `T&`，`T& &&` → `T&`；
2. **`auto` 推导**：`auto&&` 同理（`auto` 推导就是模板推导，Item 2）；
3. **`typedef` / `using` 别名**：`using A = Lref&&;`（上面的实测就是这么做的）；
4. **`decltype`**：`decltype(rx)&` 这种"decltype 产出引用类型再叠引用"的组合。

## 用折叠解释万能引用

[Item 24](item24-universal-vs-rvalue.md) 说"万能引用接左值时 `T` 推为 `T&`"，当时没解释最后一步——现在补上：

```cpp
template<class T> void f(T&& x);

Widget w;
f(w);          // T 推为 Widget&
               // 形参类型 T&& 展开为 Widget& &&
               // 折叠：& + && → &  →  形参是 Widget&，绑左值 ✓

f(Widget{});   // T 推为 Widget
               // 形参 Widget&&，无需折叠，绑右值 ✓
```

同一个 `T&&` 形参，靠"推导让 T 带引用" + "折叠把双引用压回单引用"两步，实现了左右值通吃。

## 用折叠理解 std::forward

`std::forward<T>(x)` 的本质是**按 T 的引用性条件转换**：

```cpp
template<class T>
T&& forward(std::remove_reference_t<T>& x) noexcept {
    return static_cast<T&&>(x);
}
```

- 左值转发：`T = Widget&` → 返回类型 `T&&` = `Widget& &&` → 折叠为 `Widget&` → 传出左值；
- 右值转发：`T = Widget` → 返回类型 `Widget&&` → 传出右值。

`forward` 的全部"智能"就是把 `T` 的引用性塞进 `static_cast<T&&>`，然后**让折叠规则干活**。理解了折叠，`forward` 就没有任何神秘之处——它是个一行流。

---

## 常见错误（新手踩坑）

1. **试图直接写 `int& &`**。语法错误——折叠只在四个间接语境发生，手写"引用的引用"永远是非法的。
2. **把折叠当成运行时行为**。折叠是纯编译期的类型计算，生成的代码里没有任何"折叠"动作。
3. **以为 `&& + &` 顺序影响结果**。不影响——口诀是"有 `&` 则 `&`"，与顺序无关（`& &&` 和 `&& &` 都折叠为 `&`）。
4. **把 `T&&` 的折叠和 `const` 推导混在一起**。const 不参与折叠：折叠只管引用性，`const int& &&` 折叠为 `const int&`，const 原样保留。

---

## 新手要点（和 C 的区别）

- **C 没有引用**，自然没有折叠。这是 C++ 类型系统为模板推导打的地基补丁。
- **不用背表**：记"左值引用会传染"一句就够，四条规则全是它的展开。
- **折叠是"类型代数"**：和指针运算、数组退化一样，属于"编译器替你做的类型运算"——看不见但决定行为，值得一次性搞懂终身免疫。

---

## HFT 关联

- **读框架代码的钥匙**：muduo/策略框架里 `template<class T> void setCallback(T&& f)`、`template<class... Args> void emplace(Args&&... args)` 满地都是——理解折叠才能推断每个调用的 `T` 实际推成了什么、转发后会不会意外拷贝。
- **完美转发的正确性闭环**：`T&&` 接参（Item 24）+ 折叠（本节）+ `std::forward<T>` 传出（Item 25）三者构成完整链条，缺一环就是"看似转发了实际拷了一份"的热路径隐形开销。

---

## 自测题

1. 引用折叠的四条规则是什么？用一句口诀概括。
2. 为什么 `int& &&` 不能直接写，但 `using A = Lref&&;`（`Lref = int&`）合法？
3. 折叠发生在哪四个语境？
4. 用折叠规则解释：`f(w)`（`w` 是 `Widget` 左值）调用 `template<class T> void f(T&&)` 时形参的最终类型。
5. `std::forward<T>(x)` 在 `T = Widget&` 和 `T = Widget` 时各返回什么类型？折叠在其中起了什么作用？
6. `const Widget& &&` 折叠成什么？const 参与折叠吗？

<details>
<summary>参考答案</summary>

1. `&&`：`&+&→&`、`&+&&→&`、`&&+&→&`、`&&+&&→&&`。口诀：**左值引用会传染**——组合里出现一个 `&`，结果就是 `&`；只有两个 `&&` 才保持 `&&`。已用 using 别名四组 `static_assert` 实测验证。

2. 语言规则禁止直接声明"引用的引用"，语法层面 `int& &&` 非法。但折叠规则存在的意义就是处理**间接**产生的引用的引用——别名展开、模板推导、decltype 这些语境里类型是"算"出来的，语言不禁止计算过程产生引用的引用，而是规定用折叠把它压回单引用。`using A = Lref&&;` 属于别名语境，合法且折叠为 `int&`。

3. 模板类型推导、`auto` 推导、`typedef`/`using` 别名、`decltype`。这四个也是 C++ 中仅有的能间接产生"引用的引用"的地方。

4. `T` 推为 `Widget&`（万能引用对左值实参的特殊规则），形参类型 `T&&` 展开为 `Widget& &&`，按折叠规则 `& + && → &`，最终形参类型是 `Widget&`——所以能绑定左值。

5. `T = Widget&`：返回类型 `T&&` = `Widget& &&` 折叠为 `Widget&`，传出左值；`T = Widget`：返回 `Widget&&`，传出右值。折叠是 forward 的全部机制——`static_cast<T&&>` 把 `T` 的引用性带进类型，折叠规则自动给出正确的值类别。forward 不需要运行时分支，纯类型计算。

6. `const Widget&`。const 不参与折叠——折叠只解决引用性（`&`/`&&`），cv 限定各自独立保留：`const &` 与 `&&` 折叠，引用性变 `&`，const 原样留下。

</details>

---

## 参考与延伸

- 下一节：[Item 29 移动不存在/廉价](item29-move-not-exist.md)
- 回到：[第 5 章](README.md)
