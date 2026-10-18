# Item 4：查看类型推导结果

> 第 1 章 类型推导 · Item 4 · 上一节：[Item 3 decltype](item03-decltype.md)

## 为什么要学这个（先建立直觉）

学 Item 1–3 的规则时，你心里一定闪过这个念头："规则我背了，但这段代码编译器**实际**推成了什么？"类型推导是编译器在幕后做的，推错了不会警告你——它只会安静地推成一个你没想要的类型，然后在十层调用之外报一个不知所云的错。

这一节给你的不是新规则，而是**验证手段**：怎么让编译器把推导结果吐出来。前三节学的每一条规则，都应该能用本节的方法当场验证——事实上，Item 2/3 里所有"实测"用的就是这些手段。学推导不验证，等于写代码不测试。

---

## 这节讲什么

Meyers 给了三种方法，按精度递增：编译期故意报错、运行时 `typeid`、运行时 Boost.TypeIndex。每种都附 clang 23.1.0 的真实输出。最后补两个书上没写但实用的：`static_assert` 断言法和 `__PRETTY_FUNCTION__` 法。

---

## 方法一：编译期故意报错（最轻量，零依赖）

原理：声明一个**只有声明没有定义**的类模板，用它实例化变量——编译器必须生成对象，却发现特化未定义，于是在报错信息里把模板实参（也就是你要查的类型）打印出来：

```cpp
template<typename T> struct TypeDisplay;   // 只声明，不定义

int main() {
    const int& x = 42;
    auto y = x;                    // auto 会剥掉 const&，推成什么？
    TypeDisplay<decltype(y)> td;   // 故意触发不完整类型错误
}
```

clang 23.1.0 报错原文：

```
error: implicit instantiation of undefined template 'TypeDisplay<int>'
    5 |     TypeDisplay<decltype(y)> td;
```

`TypeDisplay<int>` —— 类型就在报错里：`y` 是 `int`，`const int&` 被剥光了（印证 Item 2 的按值形态）。

**优点**：零依赖、编译期出结果、不受 RTTI 开关影响。
**缺点**：必须制造一次编译失败；报错格式因编译器而异；一次只能看一个类型。

---

## 方法二：运行时 typeid（能跑，但会骗你）

```cpp
#include <iostream>
#include <typeinfo>

int main() {
    int x = 1;
    const int& crx = x;
    const char* s = "hi";
    std::cout << typeid(x).name() << "\n";
    std::cout << typeid(crx).name() << "\n";
    std::cout << typeid(s).name() << "\n";
    std::cout << (typeid(x) == typeid(crx)) << "\n";
}
```

clang 23.1.0 实测输出：

```
i        ← int
i        ← const int& 的 const 和 & 全丢了，还是 i！
PKc      ← const char*
1        ← typeid(x) == typeid(crx)：分不出 int 和 const int&
```

两个坑，一个比一个深：

1. **输出是 mangled name**。`i` = `int`，`PKc` = `P`ointer to `K`(const) `c`(char)。这是 Itanium ABI 的内部编码，不是给人看的。可以用 `abi::__cxa_demangle` 解码：

   ```cpp
   #include <cxxabi.h>
   #include <cstdlib>
   int st = 0;
   char* d = abi::__cxa_demangle(typeid(s).name(), nullptr, nullptr, &st);
   // st == 0 时 d == "char const*"（实测）；用完要 std::free(d)
   ```

   注意 demangle 是 Itanium ABI（GCC/clang）特有的，MSVC 的 `name()` 直接给可读名字但格式另一套。

2. **`typeid` 按值语义剥掉引用和顶层 const**——和模板按值推导是同一套行为。`typeid(x)` 和 `typeid(crx)` 相等（实测输出 `1`），你根本分不出推出来的是 `int`、`int&` 还是 `const int&`。查类型推导恰恰最常要查的就是引用和 const，而 `typeid` 恰好把这些丢了。**用它验证推导结果，等于让嫌疑人自己作证。**

---

## 方法三：Boost.TypeIndex（最精确，但有依赖）

```cpp
#include <boost/type_index.hpp>

template<typename T>
void f(const T& param) {
    // type_id_with_cvr 保留 const / volatile / reference
    std::cout << boost::typeindex::type_id_with_cvr<T>().pretty_name();
}
```

`type_id_with_cvr` 是 "with const/volatile/reference" 的缩写——`typeid` 丢掉的信息它全保留，输出可读，且行为跨编译器一致。

**本机实测说明**：本机（macOS arm64）没装 Boost，这个方法没有实测输出。如果哪天真要用，Homebrew `brew install boost` 即可；但对查推导类型这件事，下面的替代方案通常够了。

---

## 补充方案 A：static_assert 断言法（推荐日常用）

不打印，直接**断言你预期的类型**——错了编译就失败，对了零开销零噪音：

```cpp
#include <type_traits>

template<typename T>
void f(T&& param) {
    static_assert(std::is_same_v<T, int&>, "T should be int& here");
}
```

这是验证推导规则最干净的方式：把"我认为它会推成 X"写成编译期断言，编译器替你判对错。Item 2/3 的全部"实测"就是这么做的。比打印法更进一步：它能进 CI，能留在代码里当文档。

## 补充方案 B：__PRETTY_FUNCTION__ 打印法（非标准但全平台可用）

GCC/clang/MSVC 都支持的编译器内建变量，打印当前函数的完整修饰签名，模板实例化后会带上实参类型：

```cpp
#include <iostream>

template<typename T>
void show(T&& x) { std::cout << __PRETTY_FUNCTION__ << "\n"; }

int main() {
    int x = 27;
    const int cx = x;
    show(x);    // void show(T &&) [T = int &]
    show(cx);   // void show(T &&) [T = const int &]
    show(27);   // void show(T &&) [T = int]
}
```

clang 23.1.0 实测输出如上（Item 2 的"完整可运行例子"用的就是它）。万能引用 + `__PRETTY_FUNCTION__`，三行代码就能看到 `T` 的精确推导结果——包括引用性和 const，这正是 `typeid` 做不到的事。非标准但三巨头都支持，调试用完全够格。

---

## 常见错误（新手踩坑）

1. **拿 `typeid` 验证推导规则**。如上，`typeid` 剥引用剥 const，用它"验证" Item 1–3 的规则会得到一堆假阴性——规则没错，是你的尺子把关键维度磨平了。
2. **以为 mangled name 是 bug**。看到 `PKc` 别慌，是 ABI 编码，demangle 一下就是 `char const*`。
3. **在生产代码里留 `typeid` 调试输出**。RTTI 有体积和运行时开销，调试完记得拆。
4. **IDE 悬停 ≠ 编译器真相**。编辑器显示的推导结果来自 IDE 自己的前端（clangd 之类），大多数时候准，但和真实编译器推导偶有出入。有分歧时以编译器为准（方法一或 static_assert）。

---

## 新手要点（和 C 的区别）

- **C 没有 typeid**。C 的类型系统是纯编译期的，运行时没有任何类型信息可查。RTTI 是 C++ 为运行时类型查询付的额外成本——所以 C++ 给了你一个关掉它的选项。
- **新手日常配置**：方法一（故意报错）+ static_assert 就够覆盖所有调试场景，不必装 Boost。IDE 悬停作为快速参考，拿不准时上编译期手段。
- **`typeid().name()` 不可信**：剥引用剥 const + mangled name，双重失真。调试类型问题别只靠它。

---

## HFT 关联

- **调试万能引用推导**：`template<class T> void f(T&& x)` 里 `T` 是 `int` 还是 `int&` 直接决定 `std::forward` 的行为——理解移动/转发的第一步就是能随时看到 `T`。用方法一或 `__PRETTY_FUNCTION__`。
- **生产代码别留 typeid，热路径直接关 RTTI**：clang 下 `-fno-rtti` 之后 `typeid` 是**编译错误**（实测：`error: use of typeid requires -frtti`），不是退化——关了 RTTI 连调试手段都没了。所以热路径的类型调试只能走编译期：`static_assert(std::is_same_v<T, U>)` 和方法一，或者单独出一个开 RTTI 的调试构建。
- **static_assert 即文档**：`static_assert(std::is_same_v<decltype(seq), uint64_t>)` 留在订单序号这类关键类型旁边，重构时类型悄悄变了会立刻编译失败——比注释可靠，比运行时断言零开销。

---

## 自测题

1. 三种查看推导结果的方法分别是什么？各自的核心缺点？
2. `typeid(x).name()` 为什么不可靠？它丢了什么信息、输出的又是什么格式？
3. `typeid(x) == typeid(crx)`（`x` 是 `int`，`crx` 是 `const int&`）的结果是 true 还是 false？这说明了什么？
4. 为什么 HFT 生产代码常用 `-fno-rtti`？clang 下关掉 RTTI 后 `typeid` 会怎样？对调试方式有什么影响？
5. 方法一（TypeDisplay 故意报错）的原理是什么？为什么说它不受 RTTI 开关影响？
6. 没有 Boost 的情况下，想看 `template<class T> void f(T&&)` 里 `T` 的精确类型（含 const 和引用），你有哪两条路？

<details>
<summary>参考答案</summary>

1. ①**编译期故意报错**：声明只声明不定义的 `template<class T> struct TypeDisplay;`，用 `TypeDisplay<decltype(x)> td;` 触发不完整类型错误，报错信息里带真实类型。缺点：必须编译失败一次、输出格式因编译器而异。②**运行时 `typeid`**：`typeid(x).name()`。缺点：要运行程序；输出是 mangled name（`PKc` 之类，需 `abi::__cxa_demangle` 解码）；且按值语义剥掉引用和顶层 const。③**Boost.TypeIndex**（`type_id_with_cvr`）：保留 const/volatile/reference、跨编译器一致，缺点是要引入 Boost 依赖、仍是运行时手段。

2. 双重失真：其一，`typeid` 的操作数按"按值"处理——引用性被剥掉、顶层 const/volatile 不参与，和模板按值推导是同一套行为，所以 `int`、`int&`、`const int&` 查出来完全一样；其二，`name()` 返回的是实现相关的 mangled name（Itanium ABI 下 `const char*` 显示为 `PKc`），不是可读类型名。要保留 cv 与引用需用 `boost::typeindex::type_id_with_cvr`，或换编译期手段。

3. true（实测输出 `1`）。说明 `typeid` 把 `const int&` 的 const 和引用全剥了，查到的只是 `int`——用它验证推导结果时，你最想确认的维度（引用性、const）恰好都被它磨掉了。

4. 关 RTTI 省掉 `type_info` 表的二进制体积和运行时开销（虚表旁的类型信息占 cache 和指令空间），也避免 `dynamic_cast`/`typeid` 引入不可预测分支。clang 下 `-fno-rtti` 后 `typeid` 是**编译错误**（`error: use of typeid requires -frtti`）——连编译都过不了。影响：热路径代码不能再用 `typeid` 打印类型，只能改用编译期手段（`static_assert(std::is_same_v<...>)`、TypeDisplay 报错法），或维护一个单独打开 RTTI 的调试构建。

5. 利用"不完整类型不能定义对象"这条语言规则：`TypeDisplay` 只有声明没有定义，`TypeDisplay<decltype(x)> td;` 要求生成对象，编译器发现特化未定义就报错，报错信息必须写出模板实参——类型就这么被套出来了。它是纯编译期机制（连对象都没生成成功），和 RTTI 是运行时设施，两者毫无关系，所以 `-fno-rtti` 不影响它。

6. ①**`__PRETTY_FUNCTION__`**：`template<typename T> void show(T&& x) { std::cout << __PRETTY_FUNCTION__ << "\n"; }`，实例化签名里带 `T` 的完整类型（`[T = const int &]`），const 和引用都在；非标准但 GCC/clang/MSVC 都支持。②**`static_assert` 断言法**：`static_assert(std::is_same_v<T, int&>)`——不打印，直接让编译器判你的预期对不对，错了编译失败。还能留在代码里进 CI。

</details>

---

## 参考与延伸

- 下一章：[第 2 章 auto](../ch02-auto/README.md)
- 回到：[第 1 章 类型推导](README.md)
