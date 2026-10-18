# Item 4：查看类型推导结果

> 第 1 章 类型推导 · Item 4 · 上一节：[Item 3 decltype](item03-decltype.md)

## 为什么要学这个（先建立直觉）

学 Item 1–3 的规则时，你心里一定闪过这个念头："规则我背了，但这段代码编译器**实际**推成了什么？"类型推导是编译器在幕后做的，推错了不会警告你——它只会安静地推成一个你没想要的类型，然后在十层调用之外报一个不知所云的错。

这一节给你的不是新规则，而是**验证手段**：怎么让编译器把推导结果吐出来。前三节学的每一条规则，都应该能用本节的方法当场验证——事实上，Item 2/3 里所有"实测"用的就是这些手段。学推导不验证，等于写代码不测试。

---

## 这节讲什么

Meyers 给了三种方法：编译期故意报错、运行时 `typeid`、Boost.TypeIndex。每种都附 clang 23.1.0 的真实输出，外加两个书上没写但更实用的补充（`static_assert` 断言法、`__PRETTY_FUNCTION__` 打印法），以及一个**不需要 RTTI 也能打印可读类型名**的小工具。最后给出"什么场景用哪种手段"的分工表。

---

## 方法一：编译期故意报错（最轻量，零依赖）

原理：声明一个**只有声明没有定义**的类模板，用它实例化变量——编译器必须生成对象，却发现特化未定义，于是在报错信息里把模板实参（也就是你要查的类型）打印出来：

```cpp
template<typename T> struct TypeDisplay;   // 只声明，不定义

int main() {
    const int  ci  = 0;
    const int& cir = ci;
    auto a1 = cir;                          // auto 按值 → ?
    auto&a2 = cir;                          // auto&      → ?
    decltype(cir) a3 = cir;                 // decltype   → ?
    TypeDisplay<decltype(a1)> t1;
    TypeDisplay<decltype(a2)> t2;
    TypeDisplay<decltype(a3)> t3;
}
```

clang 23.1.0 报错原文（一次编译同时看三个类型）：

```
error: implicit instantiation of undefined template 'TypeDisplay<int>'
error: implicit instantiation of undefined template 'TypeDisplay<const int &>'
error: implicit instantiation of undefined template 'TypeDisplay<const int &>'
```

`a1` 是 `int`——`const int&` 被剥光了（印证 Item 2 的按值形态）；`a2`/`a3` 都保住了 `const int&`。

**优点**：零依赖、编译期出结果、**不受 RTTI 开关影响**（实测 `-fno-rtti` 下同样报这三条）。
**缺点**：必须制造一次编译失败；报错格式因编译器而异。

---

## 方法二：运行时 typeid（能跑，但会骗你）

```cpp
int x = 1;
const int& crx = x;
const char* s = "hi";
std::cout << typeid(x).name() << "\n";    // i
std::cout << typeid(crx).name() << "\n";  // i  ← const 和 & 全丢了
std::cout << typeid(s).name() << "\n";    // PKc
std::cout << (typeid(x) == typeid(crx));  // 1
```

clang 23.1.0 实测输出：`i` / `i` / `PKc` / `1`。两个坑，一个比一个深：

1. **输出是 mangled name**。`i` = `int`，`PKc` = `P`ointer to `K`(const) `c`(char)。这是 Itanium ABI 的内部编码。可以用 `abi::__cxa_demangle(typeid(s).name(), nullptr, nullptr, &st)` 解码，实测 `st == 0` 时得到 `"char const*"`（用完要 `std::free(d)`）。注意 demangle 是 Itanium ABI（GCC/clang）特有的，MSVC 的 `name()` 直接给可读名字但格式另一套。
2. **`typeid` 按值语义剥掉引用和顶层 const**——和模板按值推导是同一套行为。`typeid(x)` 和 `typeid(crx)` 相等（实测输出 `1`），你根本分不出推出来的是 `int`、`int&` 还是 `const int&`。查类型推导恰恰最常要查的就是引用和 const，而 `typeid` 恰好把这些丢了。**用它验证推导结果，等于让嫌疑人自己作证。**

### 另一个反直觉点：多态类上查的是动态类型

对**有虚函数**的类，`typeid(*ptr)` 查对象的**动态类型**；没有虚函数时只查**静态类型**。clang 23.1.0 实测（demangle 后）：

```
polymorphic     typeid(*p) : Derived   ← 通过基类指针看穿了真实类型
non-polymorphic typeid(*q) : NBase     ← 只看静态类型，看不穿
```

这说明 `typeid` 的设计目标是 RTTI 下探，不是查推导类型——用错场景就会得出两套语义混杂的结论。

---

## 方法三：Boost.TypeIndex（最精确，但有依赖）

```cpp
#include <boost/type_index.hpp>
template<typename T>
void f(const T& param) {
    std::cout << boost::typeindex::type_id_with_cvr<T>().pretty_name();
}
```

`type_id_with_cvr` 是 "with const/volatile/reference" 的缩写——`typeid` 丢掉的信息它全保留，输出可读，且行为跨编译器一致。

**本机实测说明**：本机（macOS arm64）没装 Boost，实测 `fatal error: 'boost/type_index.hpp' file not found`（`/opt/homebrew/include/boost`、`/usr/local/include/boost` 均不存在），所以这个方法在本机没有输出。真要用 `brew install boost` 即可；但对查推导类型这件事，下面的零依赖替代方案通常够用。

---

## 补充方案 A：static_assert 断言法（推荐日常用）

不打印，直接**断言你预期的类型**——错了编译就失败，对了零开销零噪音：

```cpp
template<typename T>
void f(T&& param) { static_assert(std::is_same_v<T, int&>, "T should be int& here"); }
```

这是验证推导规则最干净的方式：把"我认为它会推成 X"写成编译期断言，编译器替你判对错。Item 2/3 的全部"实测"就是这么做的。比打印法更进一步：它能进 CI，能留在代码里当文档。

## 补充方案 B：__PRETTY_FUNCTION__ 打印法（非标准但全平台可用）

GCC/clang/MSVC 都支持的编译器内建变量，打印当前函数完整签名，模板实例化后带上实参类型：

```cpp
template<typename T> void show(T&& x) { std::cout << __PRETTY_FUNCTION__ << "\n"; }
int x = 27; const int cx = x;
show(x);    // void show(T &&) [T = int &]
show(cx);   // void show(T &&) [T = const int &]
show(27);   // void show(T &&) [T = int]
```

clang 23.1.0 实测输出如上。万能引用 + `__PRETTY_FUNCTION__`，三行代码就能看到 `T` 的精确推导结果——包括引用性和 const，这正是 `typeid` 做不到的事。

## 补充方案 C：自己写可读类型名打印器（无需 RTTI）

把方案 B 的字符串切一刀，就得到一个"打印任意类型可读名字"的工具，对**关掉 RTTI 的构建**尤其有用：

```cpp
template<class T>
std::string type_name() {
    std::string_view sv = __PRETTY_FUNCTION__;   // "std::string type_name() [T = const int &]"
    auto b = sv.find("[T = ") + 5;
    auto e = sv.rfind(']');
    return std::string(sv.substr(b, e - b));
}
```

clang 23.1.0 实测：`type_name<decltype(x)>()` → `int`，`type_name<decltype((x))>()` → `int &`，`type_name<decltype(crx)>()` → `const int &`，`type_name<decltype(v.begin())>()` → `std::__wrap_iter<int *>`。**`-fno-rtti` 下输出完全一致**（把只含 `type_name` 的程序用 `-fno-rtti` 编译，实测输出不变）——它是纯编译期字符串操作，和运行时类型信息无关。反过来说：下面完整例子里那几行 `typeid` 在 `-fno-rtti` 下会直接编译失败，正好演示了"关 RTTI 就丢了运行期手段"。

---

## 手段分工：什么时候用哪个

| 场景 | 推荐手段 | 理由 |
|------|---------|------|
| 一次性查某个类型是什么 | 方法一（TypeDisplay 报错） | 零依赖，立刻出结果 |
| 验证"我推得对不对"、想留进 CI | `static_assert(is_same_v<...>)` | 编译期判定，零运行开销，可长期留存 |
| 想在日志里打印类型名 | 方案 C（`type_name<T>()`） | 可读、无需 RTTI |
| 生产构建（`-fno-rtti`） | 方法一 / static_assert / 方案 C | `typeid` 直接编译失败，不能用 |
| 需要跨编译器一致的可读输出 | Boost.TypeIndex | 唯一保证，但要引入依赖 |
| 快速瞄一眼 | IDE 悬停 | 快，但不可全信 |

**IDE 悬停 ≠ 编译器真相**：编辑器显示的推导结果来自 IDE 自己的前端（clangd 之类），大多数时候准，但和真实编译器推导偶有出入（尤其模板元编程、SFINAE 场景）。有分歧时以编译器为准——用方法一或 `static_assert` 做终裁。分工原则：**IDE 负责"快速导航"，编译期手段负责"下结论"**。

---

## 完整可运行例子

```cpp
#include <cxxabi.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <vector>

// 手段 1（编译期）：只声明不定义的类模板 → 报错里带类型
template<typename T> struct TypeDisplay;
// 手段 2（运行期）：typeid + demangle
static std::string demangle(const char* n) {
    int st = 0;
    char* d = abi::__cxa_demangle(n, nullptr, nullptr, &st);
    std::string r = (st == 0 && d) ? d : n;
    std::free(d);
    return r;
}
// 手段 3（编译期，无需 RTTI）：反解 __PRETTY_FUNCTION__
template<class T>
std::string type_name() {
    std::string_view sv = __PRETTY_FUNCTION__;
    auto b = sv.find("[T = ") + 5;
    auto e = sv.rfind(']');
    return std::string(sv.substr(b, e - b));
}

int main() {
    int x = 27;
    const int  cx  = x;
    const int& crx = x;
    std::cout << "-- typeid（丢 cv/ref）--\n";
    std::cout << "typeid(x)   : " << demangle(typeid(x).name()) << "\n";
    std::cout << "typeid(crx) : " << demangle(typeid(crx).name()) << "\n";
    std::cout << "equal? " << (typeid(x) == typeid(crx)) << "\n";
    std::cout << "-- type_name<decltype>（保 cv/ref）--\n";
    std::cout << "decltype(x)   : " << type_name<decltype(x)>() << "\n";
    std::cout << "decltype((x)) : " << type_name<decltype((x))>() << "\n";
    std::cout << "decltype(cx)  : " << type_name<decltype(cx)>() << "\n";
    std::cout << "decltype(crx) : " << type_name<decltype(crx)>() << "\n";
    std::vector<int> v{1, 2, 3};
    std::cout << "decltype(v[0])      : " << type_name<decltype(v[0])>() << "\n";
    std::cout << "decltype(v.begin()) : " << type_name<decltype(v.begin())>() << "\n";
    static_assert(std::is_same_v<decltype(crx), const int&>);   // 手段 4
    // TypeDisplay<decltype(crx)> td;                           // 手段 1：解开看报错
}
```

本机实测 clang 23.1.0（`-std=c++17 -Wall`）输出：

```
-- typeid（丢 cv/ref）--
typeid(x)   : int
typeid(crx) : int
equal? 1
-- type_name<decltype>（保 cv/ref）--
decltype(x)   : int
decltype((x)) : int &
decltype(cx)  : const int
decltype(crx) : const int &
decltype(v[0])      : int &
decltype(v.begin()) : std::__wrap_iter<int *>
```

同一组变量，`typeid` 把 `int`/`const int&` 一律报成 `int`，而 `type_name<decltype(...)>` 把引用和 const 原样呈现——**这就是选错工具会得到的两种结论**。解开最后一行注释，报错原文是 `error: implicit instantiation of undefined template 'TypeDisplay<const int &>'`。

---

## 常见错误（新手踩坑）

1. **拿 `typeid` 验证推导规则**。如上，`typeid` 剥引用剥 const，用它"验证" Item 1–3 的规则会得到一堆假阴性——规则没错，是你的尺子把关键维度磨平了。
2. **以为 mangled name 是 bug**。看到 `PKc` 别慌，是 ABI 编码，demangle 一下就是 `char const*`。
3. **在生产代码里留 `typeid` 调试输出**。RTTI 有体积和运行时开销，调试完记得拆。
4. **IDE 悬停 ≠ 编译器真相**。编辑器前端（clangd）与真实编译器偶有出入，有分歧用编译期手段终裁。
5. **以为关掉 RTTI 只是"少点信息"**。clang 下 `-fno-rtti` 之后 `typeid` 是**编译错误**（实测 `error: use of typeid requires -frtti`），`dynamic_cast` 同样（`error: use of dynamic_cast requires -frtti`）；但虚函数、虚表、`__PRETTY_FUNCTION__`、`static_assert` 全部照常工作（实测虚调用仍输出 `2`）。所以关 RTTI 之前要先把调试手段换成编译期的。
6. **把 `typeid(*p)` 当静态类型看**。多态类上它查的是动态类型（实测输出 `Derived`），非多态类上才是静态类型（实测输出 `NBase`）——两套语义混着用极易误判。

---

## HFT 关联

- **调试万能引用推导**：`template<class T> void f(T&& x)` 里 `T` 是 `int` 还是 `int&` 直接决定 `std::forward` 的行为——理解移动/转发的第一步就是能随时看到 `T`。用方法一或 `__PRETTY_FUNCTION__` / `type_name<T>()`。
- **生产代码别留 typeid，热路径直接关 RTTI**：clang 下 `-fno-rtti` 之后 `typeid` 是编译错误，不是退化——关了 RTTI 连调试手段都没了。所以热路径的类型调试只能走编译期：`static_assert(std::is_same_v<T, U>)`、方法一，或本文的 `type_name<T>()`（实测 `-fno-rtti` 下输出不变）；也可以单独维护一个开 RTTI 的调试构建。
- **static_assert 即文档**：`static_assert(std::is_same_v<decltype(seq), uint64_t>)` 留在订单序号这类关键类型旁边，重构时类型悄悄变了会立刻编译失败——比注释可靠，比运行时断言零开销。
- **别把体积结论当常识**：本机对一个只有两个类的示例程序做 `-O2` 对比，`-fno-rtti` 版本反而略大（52672 → 53056 字节）——类少时 RTTI 表几乎不占空间，节区对齐噪声盖过了它。关 RTTI 的真正收益是禁掉不可预测的 `dynamic_cast` 分支，别拿体积当理由。

---

## 自测题

**1. 三种查看推导结果的方法分别是什么？各自的核心缺点？**
<details><summary>答案</summary>
①**编译期故意报错**：`template<class T> struct TypeDisplay;` 只声明不定义，`TypeDisplay<decltype(x)> td;` 触发不完整类型错误，报错里带真实类型。缺点：必须编译失败一次、输出格式因编译器而异。②**运行时 `typeid`**：`typeid(x).name()`。缺点：输出是 mangled name（需 `abi::__cxa_demangle`），且按值语义剥掉引用和顶层 const。③**Boost.TypeIndex**（`type_id_with_cvr`）：保留 cv/引用、跨编译器一致，缺点是要引入 Boost 依赖（本机未装，实测 `file not found`）。
</details>

**2. `typeid(x).name()` 为什么不可靠？它丢了什么信息、输出的又是什么格式？**
<details><summary>答案</summary>
双重失真：其一，`typeid` 的操作数按"按值"处理——引用性被剥掉、顶层 const/volatile 不参与，所以 `int`、`int&`、`const int&` 查出来完全一样（实测 `typeid(x) == typeid(crx)` 为 `1`）；其二，`name()` 返回实现相关的 mangled name（Itanium ABI 下 `const char*` 显示为 `PKc`），不是可读类型名。
</details>

**3. `typeid(x) == typeid(crx)`（`x` 是 `int`，`crx` 是 `const int&`）的结果是 true 还是 false？这说明了什么？**
<details><summary>答案</summary>
true（实测输出 `1`）。说明 `typeid` 把 `const int&` 的 const 和引用全剥了，查到的只是 `int`——用它验证推导结果时，你最想确认的维度（引用性、const）恰好都被它磨掉了。
</details>

**4. 为什么 HFT 生产代码常用 `-fno-rtti`？clang 下关掉 RTTI 后 `typeid` 会怎样？哪些手段还能用？**
<details><summary>答案</summary>
关 RTTI 是为了禁掉 `dynamic_cast`/`typeid` 引入的不可预测分支，也省掉类型信息表。clang 下 `-fno-rtti` 后 `typeid` 是**编译错误**（实测 `error: use of typeid requires -frtti`），`dynamic_cast` 同样（`error: use of dynamic_cast requires -frtti`）。还能用的：虚函数/虚表（实测虚调用仍正常返回 `2`）、`static_assert`、TypeDisplay 报错法、`__PRETTY_FUNCTION__` 与 `type_name<T>()`（实测输出与开 RTTI 时完全一致）。
</details>

**5. 方法一（TypeDisplay 故意报错）的原理是什么？为什么说它不受 RTTI 开关影响？**
<details><summary>答案</summary>
利用"不完整类型不能定义对象"这条语言规则：`TypeDisplay` 只有声明没有定义，`TypeDisplay<decltype(x)> td;` 要求生成对象，编译器发现特化未定义就报错，报错信息必须写出模板实参——类型就这么被套出来了。它是纯编译期机制（连对象都没生成成功），和 RTTI 这个运行时设施毫无关系，所以 `-fno-rtti` 下实测同样报三条 `implicit instantiation of undefined template`。
</details>

**6. 没有 Boost 的情况下，想看 `template<class T> void f(T&&)` 里 `T` 的精确类型（含 const 和引用），你有哪三条路？**
<details><summary>答案</summary>
①**`__PRETTY_FUNCTION__`**：实例化签名里带 `T` 的完整类型（`[T = const int &]`），const 和引用都在，非标准但 GCC/clang/MSVC 都支持。②**`static_assert(std::is_same_v<T, int&>)`**：不打印，直接让编译器判你的预期对不对，还能留在代码里进 CI。③**`type_name<T>()` 小工具**：切一刀 `__PRETTY_FUNCTION__` 的字符串，得到可读类型名（实测 `const int &`），且 `-fno-rtti` 下照样能用。
</details>

**7. `typeid(*p)` 在多态类和非多态类上分别查到什么？**
<details><summary>答案</summary>
多态类（有虚函数）上查到对象的**动态类型**：`Base* p = &derived;` 时 `typeid(*p)` 实测输出 `Derived`。非多态类上只查**静态类型**：`NBase* q = &nDerived;` 时 `typeid(*q)` 实测输出 `NBase`，看不穿。这也是"typeid 只在多态类上做运行时下探"的含义——用它查推导类型属于用错场景。
</details>

---

## 参考与延伸

- 上一节：[Item 3 理解 decltype](item03-decltype.md)
- 下一章：[第 2 章 auto](../ch02-auto/README.md)
- 相关：[Item 1 理解模板类型推导](item01-template-type-deduction.md)（本节方法用于验证 Item 1–3 的全部规则）
- 回到：[第 1 章 类型推导](README.md)
