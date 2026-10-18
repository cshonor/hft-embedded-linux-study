# Item 27：万能引用重载的替代方案

> 第 5 章 右值引用、移动语义与完美转发 · Item 27 · 上一节：[Item 26 避免万能引用重载](item26-avoid-overloading-universal.md) · 下一节：[Item 28 引用折叠](item28-reference-collapsing.md)

## 为什么要学这个（先建立直觉）

Item 26 说"别给万能引用开重载"——它太贪婪，连拷贝构造都可能抢不过：拷贝构造形参是 `const Person&`，绑**非 const 左值**要追加一次 const 转换；模板 `T=Person&` 使形参折叠成 `Person&`，是零转换的精确匹配，于是胜出（const 左值两边都精确匹配 → 非模板优先，所以劫持只在这条路上发生）。但需求不会消失。这一节给四条路：绕开重载决议（标签分发）、给模板上镣铐（enable_if / Concepts）、按值接参 + move，以及应当**优先考虑**的"干脆别用模板"。

问题现场，Meyers 的经典例子（本机实测 clang 23.1.0）：

```cpp
class Person {
public:
    template<typename T>
    explicit Person(T&& n) : name(std::forward<T>(n)) {}
    Person(const Person& rhs) : name(rhs.name) {}
    std::string name;
};
Person p2("x");
Person p3(p2);   // p2 是非 const 左值
```

`-std=c++17 -Wall` 报错原文：

```
error: no matching constructor for initialization of 'std::string' (aka 'basic_string<char>')
    explicit Person(T&& n) : name(std::forward<T>(n)) {}
                             ^    ~~~~~~~~~~~~~~~~~~
note: in instantiation of function template specialization 'Person::Person<Person &>' requested here
    Person p3(p2);
           ^
```

最后一行 `Person::Person<Person &>` 是**唯一的破案线索**——调用方写的是"拷贝一个 Person"，报错却落在 `std::string` 的构造上。真实项目里换个容器（`vector`/`optional`/自定义 ring），错误会翻好几屏。本节要消灭的就是这类"错误位置离真凶很远"的诊断。

---

## 这节讲什么

1. 方案一 **标签分发**：把分发决策从重载决议手里抢回来；
2. 方案二 **enable_if**（C++11/14/17 正解）与方案三 **Concepts**（C++20 正解）——含派生类漏网这个进阶坑；
3. 方案四 **按值接参 + move**：什么前提下值得，实测多花一次移动；
4. 方案零（默认）**具名重载**：为什么它该先被考虑而不是最后兜底。

全部论断附本机实测（clang 23.1.0，arm64-apple-darwin，libc++，默认 `-std=c++17 -Wall`，C++20 段落标 `-std=c++20`）。

---

## 方案一：标签分发（tag dispatch）

思路：万能引用入口保留（来者不拒），但**不在入口层面做重载**——把分发推迟到内部，用一个编译期"标签"参数走普通重载决议：

```cpp
std::multiset<std::string> names;
template<class T>
void logAndAddImpl(T&& name, std::false_type) { names.emplace(std::forward<T>(name)); }  // 名字通道
void logAndAddImpl(int idx, std::true_type)   { names.emplace(nameFromIdx(idx)); }       // 索引通道
template<class T>
void logAndAdd(T&& name) {
    logAndAddImpl(std::forward<T>(name),
                  std::is_integral<typename std::remove_reference<T>::type>());
}
```

`std::true_type` / `std::false_type` 是两个不同的空类——两个 Impl 靠**第二个参数的类型**区分，走最普通、最可预测的重载决议，与"万能引用抢不抢"完全无关。实测输出：

```
名字通道
名字通道
索引通道 idx=22
索引通道 idx=7
names={ AAPL MSFT name-of-22 name-of-7 }
```

**贪婪只发生在入口一次**：进入函数体后分发由标签类型决定，万能引用不再参与竞争。另外两个细节——标签里必须剥引用（下面两组只差 `remove_reference`）：

```cpp
template<class T> void pickOk (T&& v) { pickImpl(std::forward<T>(v), std::is_integral<std::remove_reference_t<T>>()); }
template<class T> void pickBug(T&& v) { pickImpl(std::forward<T>(v), std::is_integral<T>()); }
int idx = 7;
pickOk(55);    // [整型通道]
pickOk(idx);   // [整型通道]   ← 正确
pickBug(idx);  // [通用通道]   ← T=int&，is_integral<int&> 恒假
```

实测输出：`pickOk(55) [整型通道]` / `pickOk(idx) [整型通道]` / `pickBug(idx) [通用通道]`。**这和 enable_if 必须配 `decay_t` 是同一个坑的两个马甲**：万能引用接左值时 `T` 自带 `&`，任何"直接看 `T`"的 trait 判定都会失效。

**它没过时**：`if constexpr` 能简化内部实现，但分发条件复杂、要 `static_assert`、要留扩展点时标签分发仍最显式——标准库 `std::advance` 按迭代器类别分发就是它的活化石。**版本兼容**上，标签分发是 C++11 就有的机制，但 `remove_reference_t` 这类 `_t` 别名到 C++14 才进标准——实测 `-std=c++11` 下报 `error: no template named 'remove_reference_t' in namespace 'std'`，`c++14`/`c++20` 通过。

---

## 方案二：enable_if 约束模板（C++11/14/17）

反过来想：不给万能引用开重载，而是**给它上镣铐**——在某些类型面前它直接"不存在"（SFINAE）：

```cpp
class Fixed {
public:
    template<typename T,
             typename = std::enable_if_t<
                 !std::is_same_v<std::decay_t<T>, Fixed> &&
                 !std::is_base_of_v<Fixed, std::decay_t<T>> >>
    explicit Fixed(T&& n) : name(std::forward<T>(n)) {}
    Fixed(const Fixed& rhs) : name(rhs.name) { /* 拷贝构造 */ }
    Fixed(Fixed&& rhs) noexcept : name(std::move(rhs.name)) { /* 移动构造 */ }
    std::string name;
};
```

实测：非 const 左值 `Fixed f2(f1)` 输出 `Fixed 拷贝构造`，`Fixed f4(std::move(f3))` 输出 `Fixed 移动构造`——劫持消失，特殊成员函数各司其职。构造函数没有返回类型，`enable_if` 只能挂**模板默认实参**，不能放返回值处。

### 漏掉 decay_t 会怎样（实测对照）

约束写成 `!std::is_same_v<T, Bad>`（不带 `decay_t`），同样的调用：

```
error: no matching constructor for initialization of 'std::string' (aka 'basic_string<char>')
note: in instantiation of function template specialization 'Bad::Bad<Bad &, void>' requested here
    Bad b(a);
```

**报错与未加约束时一字不差**——约束在 `T=Bad&` 时恒为真，从未生效过。这是"加了 enable_if 怎么还报错"的第一答案：删掉 `decay_t` 对比报错，没变化就说明它没生效。

### 派生类漏网（进阶坑）

即使加了 `decay_t`，也只挡住"正好是本类"。实测 `Base copy_of_d(d)`（`Derived d`）输出：

```
Base 模板构造 [T=7Derived]     ← typeid(...).name() 的修饰名，派生类被误吞
```

补上 `!std::is_base_of_v<Base, std::decay_t<T>>` 后实测变为：

```
Base 模板构造 [T=i]     ← Derived() 构造 Base 部分时的 Base(0)
Base 拷贝构造            ← copy_of_d(d) 正确走到拷贝构造
```

结论：通用约束的标准形式是**同时排除本类和基类**，`is_base_of` 的基类放第一个实参（写反会漏判）。

---

## 方案三：C++20 Concepts（新项目默认答案）

`requires` 语义与 enable_if 相同，但约束写在明处、**可命名、可 `&&` 组合、可复用**（clang 23.1.0 + `-std=c++20` 实测通过）：

```cpp
template<class T, class Self>
concept NotSelf = !std::is_same_v<std::decay_t<T>, Self>;
template<class T>
concept StringLike = requires(T&& t) { std::string(std::forward<T>(t)); };
class PersonC {
public:
    template<typename T> requires NotSelf<T, PersonC> && StringLike<T>
    explicit PersonC(T&& n) : name(std::forward<T>(n)) {}
    PersonC(const PersonC&) = default;
    PersonC(PersonC&&) noexcept = default;
    std::string name;
};
```

实测 `PersonC b(a);`（非 const 左值）走拷贝构造，`std::cout << b.name` 输出 `alice`。

诊断方面，clang 23.1.0 上两者**都可追究到一个 statement**，Concepts 不是"报错更短"而是"更结构化"：

```
Concepts:  note: candidate template ignored: constraints not satisfied [with T = std::vector<int> &]
           note: because 'std::vector<int> &' does not satisfy 'StringLike'
enable_if: note: candidate template ignored: requirement
                 'std::is_constructible_v<std::string, std::vector<int> &>'
                 was not satisfied [with T = std::vector<int> &]
```

两者首行都是 `error: no matching constructor for initialization of 'PersonX'`。Concepts 真正拉开差距的是 GCC 9 及更早——那才是 enable_if 展开成几十行实例化噪声的年代。

---

## 方案四：按值接参 + move（有前提）

还有一种"绕开重载"的思路：**压根不写万能引用**，参数按值接，函数体里 `std::move` 到目标位置：

```cpp
template<class T> void setFwd(T&& v) { p_ = std::forward<T>(v); }   // A
void setByValue(Payload v)          { p_ = std::move(v); }          // B
```

本机实测 clang 23.1.0（`-O0`，`Payload` 内含超过 SSO 长度的 `std::string`）：

| 实参形态 | A `setFwd(T&&)` | B `setByValue(Payload)` |
|---|---|---|
| 左值 | copy-assign ×1 | copy-ctor ×1 + move-assign ×1 |
| prvalue（`Payload{}`） | move-assign ×1 | move-assign ×1（prvalue 直接构造进形参） |
| 具名右值（`std::move(lv)`） | move-assign ×1 | move-ctor ×1 + move-assign ×1 |

对应计数输出：`A setFwd(左值) | copy-assign=1`、`B setByValue(左值) | copy=1 move-assign=1`、`A setFwd(具名右值) | move-assign=1`、`B setByValue(具名右值) | move=1 move-assign=1`。

**结论**：按值接参在 3 种实参形态里有 2 种**多付一次移动**。它值得用的前提是移动足够廉价、可读性收益明显、不需要区分左右值——典型场景是 Meyers 在 Item 41 说的**构造函数**（必留一份，forward 省不掉那次拷贝）。像上面"赋值到已有成员"的用法纯亏一次移动：**热路径上 `Tick(uint64_t ts)` 按值可以，tick buffer 按值是要出事的。**

---

## 方案零：具名重载（默认先试这个）

```cpp
void set(const std::string& s);
void set(int idx);
void set(OrderId id);
```

为什么它该被优先考虑：**① 它做隐式转换，约束模板不做**；**② 它不可能被劫持**（不进模板候选集）；**③ 它能吃 `T&&` 吃不下**的实参（`set({1,2,3})`、`set(nullptr)`，见 [Item 30](item30-forwarding-failures.md)）；**④ 报错聚焦 overload set**，没有实例化栈。实测：

```cpp
struct Meters { Meters(double); };
void setDistance(const Meters&);                                     // 非模板
template<class T> requires std::is_arithmetic_v<std::remove_reference_t<T>>
void setDistanceT(T&&);                                              // 约束模板
setDistance(10);      // OK：int → Meters（隐式转换）
setDistance(1.5f);    // OK：float → Meters
setDistanceT(10);     // OK
setDistanceT(Meters{2});
// error: no matching function for call to 'setDistanceT'
// note: candidate template ignored: constraints not satisfied [with T = Meters]
// note: because 'std::is_arithmetic_v<std::remove_reference_t<Meters>>' evaluated to false
```

模板推导是"精确"的：它推实参自己的类型，不会替你转换成目标类型。这是模板方案永远补不回来的能力差。

---

## 完整可运行例子

四条方案收在一个文件里（clang 23.1.0，`-std=c++17 -Wall`，零警告）：

```cpp
#include <iostream>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
std::multiset<std::string> names;
// ===== 方案一：标签分发 =====
void addImpl(std::string&& s, std::false_type) {                    // 名字通道
    std::cout << "[标签] 名字: " << s << "\n";  names.emplace(std::move(s));
}
void addImpl(int idx, std::true_type) {                             // 索引通道
    std::cout << "[标签] 索引: " << idx << "\n"; names.emplace("sym-" + std::to_string(idx));
}
template<class T>
void addTag(T&& v) { addImpl(std::forward<T>(v), std::is_integral<std::remove_reference_t<T>>()); }
// ===== 方案二：enable_if =====
class Person {
public:
    template<class T, class = std::enable_if_t<
        !std::is_same_v<std::decay_t<T>, Person> &&
        !std::is_base_of_v<Person, std::decay_t<T>>>>
    explicit Person(T&& n) : name(std::forward<T>(n)) { std::cout << "[约束] 模板构造\n"; }
    Person(const Person& o) : name(o.name) { std::cout << "[约束] 拷贝构造\n"; }
    std::string name;
};
// ===== 方案四：按值接参 + move =====
struct Book { std::string title; void retitle(std::string t) { title = std::move(t); } };
int main() {
    addTag("AAPL");          // false_type → 名字通道
    addTag(7);               // true_type  → 索引通道
    int idx = 9;  addTag(idx);            // 剥了引用，仍走索引通道
    Person p1("tom");  Person p2(p1);     // decay_t 生效 → 拷贝构造（不再被劫持）
    Book b;  std::string t = "EMC++";
    b.retitle(t);            // 进参一次拷贝 + 一次移动赋值
    std::cout << b.title << "\n";
}
```

本机实测 clang 23.1.0 输出：

```
[标签] 名字: AAPL
[标签] 索引: 7
[标签] 索引: 9
[约束] 模板构造
[约束] 拷贝构造
EMC++
```

---

## 常见错误（新手踩坑）

1. **enable_if / 标签判定忘写 `decay_t` / `remove_reference`**。实测：加了约束后报错原文与没加完全一致（`Bad::Bad<Bad &, void>`）。
2. **只排除本类不排除基类**。实测 `Base copy_of_d(d)` 输出 `Base 模板构造 [T=7Derived]`，需补 `!std::is_base_of_v<Self, std::decay_t<T>>`。
3. **给构造函数用 enable_if 时放错位置**。构造函数没有返回类型，`enable_if` 只能挂模板默认实参。
4. **用 `-std=c++11` 编写着 `_t` 别名的代码**。实测报 `no template named 'remove_reference_t' in namespace 'std'`——`_t` 别名是 C++14 引进的。
5. **能用具名重载的硬上模板**。除了把问题复杂化，还会**丢掉隐式转换**（实测 `setDistance(1.5f)` 通过、`setDistanceT(Meters{2})` 被拦）。
6. **以为 pass-by-value + move 与 forward 等价**。实测赋值语义下 3 种实参形态有 2 种多付一次移动。

---

## HFT 关联

- **行情分发接入口**：`template<class T> void onMessage(T&& msg)` 按消息类型分发解码器——用 Concepts（`requires Decodable<T>`）或标签分发，避免万能引用吞掉本该精确匹配的专门重载。handler 错配是静默的（能编译、跑错分支），比编不过贵得多。
- **小值类型的构造函数**：`Position(Symbol s, Qty q)` 这种"参数可移动且便宜"的类型，按值接参 + move 最省心；但 `setXxx()` 这类**赋值**接口别用，实测多付一次移动。
- **热路径值类型别开万能引用重载**：`Tick(uint64_t ts)` / `Tick(const Tick&)` 用具名重载就够——一旦掺进 `T&&`，每个调用点都要在脑内跑一遍重载决议 + 引用折叠。

---

## 自测题

**1. `Person p3(p2)`（`p2` 非 const 左值）为什么被万能引用构造劫持？报错里哪一行是破案线索？**

<details><summary>答案</summary>
拷贝构造形参 `const Person&` 绑非 const 左值要追加一次 const 转换；模板 `T=Person&` 折叠成 `Person&`，零转换精确匹配 → 胜出。模板体执行 `std::string name(std::forward<Person&>(p2))` 失败。破案线索是最后一行的 `note: in instantiation of function template specialization 'Person::Person<Person &>' requested here`——错误文本主体却落在 string 构造上。看转发相关报错，**永远先翻到最后几条 note 找 `[T = ...]`**。
</details>
**2. 标签分发怎么绕开"万能引用贪婪匹配"？标签本身是什么？**

<details><summary>答案</summary>
万能引用只做入口（来者不拒但不决策），函数体里把实参 `forward` 给一个**带编译期标签参数**的 impl 重载集。标签是空类（`std::true_type`/`std::false_type` 或 trait 产出的类型），两重载靠第二个形参的**类型**区分，走最普通的重载决议。实测：`addTag(7)` 走索引通道、`addTag("AAPL")` 走名字通道。
</details>
**3. 为什么 enable_if 约束必须写 `std::decay_t<T>`？漏了观察到什么现象？**

<details><summary>答案</summary>
接左值时 `T` 推成 `Person&`，直接 `is_same_v<T, Person>` 恒假 → 约束从未生效 → SFINAE 没把候选移除。实测现象：加了约束后**报错原文与没加时一字不差**（`no matching constructor for initialization of 'std::string'` + `Bad<Bad &, void>`）。
</details>
**4. 加了 `decay_t` 之后还有什么类型能漏进约束模板？怎么补？**

<details><summary>答案</summary>
**派生类**。`decay_t<Derived>` 是 `Derived` 而非 `Base`，条件照样成立。实测 `Base copy_of_d(d)` 输出 `Base 模板构造 [T=7Derived]`；补 `!std::is_base_of_v<Self, std::decay_t<T>>` 后输出变成 `Base 拷贝构造`。
</details>
**5. C++20 Concepts 相对 enable_if 真正强在哪？实测报错差异多大？**

<details><summary>答案</summary>
语义等价（不满足即从候选集消失），真正优势是约束**有名字、可 `&&` 组合、可复用**。实测 clang 23.1.0 上两者都可追究到一个 statement——Concepts 是 `constraints not satisfied [with T = ...]` + `because ... does not satisfy 'StringLike'`，enable_if 是 `requirement '...' was not satisfied [with T = ...]`。**不是"更短"而是更结构化**。
</details>
**6. 什么情况选"按值接参 + move"而不是万能引用转发？给实测数据。**

<details><summary>答案</summary>
前提是"移动廉价且必须留一份拷贝"，典型是**构造函数**。实测 `-O0`：`setFwd` 左值 1 copy-assign、prvalue 1 move-assign、具名右值 1 move-assign；`setByValue` 左值 1 copy-ctor + 1 move-assign、prvalue 1 move-assign、具名右值 1 move-ctor + 1 move-assign。**赋值语义下 3 种形态有 2 种多付一次移动**。
</details>
**7. 为什么 Meyers 把"用具名重载"当默认方案而非权宜之计？**

<details><summary>答案</summary>
因为约束模板仍然会丢东西：① 推导精确，**不做目标类型的隐式转换**——实测 `setDistance(1.5f)` 通过，算术约束模板却把 `Meters{2}` 拦在门外；② 模板吃不下 `{}`、重载函数名、位字段这些实参（Item 30）；③ 报错聚焦 overload set，不用翻实例化栈。
</details>

---

## 参考与延伸

- 上一节：[Item 26 避免万能引用重载](item26-avoid-overloading-universal.md)（本节要解决的问题）
- 下一节：[Item 28 引用折叠](item28-reference-collapsing.md)（`T&&` 为什么会变成 `T&`）
- 相关：[Item 30 完美转发失败的处境](item30-forwarding-failures.md)（模板方案的剩余边界）
- 相关：[第 8 章 Item 41 按值传递](../ch08-tweaks/item41-pass-by-value.md)（方案四的完整论述）
- 回到：[第 5 章 右值引用、移动语义与完美转发](README.md)
