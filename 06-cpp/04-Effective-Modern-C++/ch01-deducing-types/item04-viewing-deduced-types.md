# Item 4：查看类型推导结果

> 第 1 章 类型推导 · Item 4 · 上一节：[Item 3 decltype](item03-decltype.md)

## 这节讲什么

类型推导出了 bug，怎么确认编译器到底推成了什么？Meyers 给了三种方法，从编译期到运行时，精度递增。

---

## 三种方法

### 1. 编译期报错（最轻量）

故意制造一个类型不匹配的编译错误：
```cpp
template<class T> class TypeDisplay;
auto x = ...;
TypeDisplay<decltype(x)> td;  // 编译报错：undefined specialization
// 报错信息里会显示 T 的真实类型
```
缺点：报错信息冗长，不同编译器格式不同。

### 2. 运行时 RTTI（`typeid`）

```cpp
std::cout << typeid(x).name() << std::endl;
```
缺点：①输出是编译器内部的 mangled name（如 `PKc` = `const char*`），需要 `abi::__cxa_demangle` 解码；②`typeid` 对引用会退化，丢掉 `const`/`&` 信息——不可靠。

### 3. 运行时 Boost.TypeIndex（最精确）

```cpp
#include <boost/type_index.hpp>
std::cout << boost::typeindex::type_id_with_cvr<T>().pretty_name();
```
`type_id_with_cvr` 保留 const/volatile/reference，输出可读。不依赖编译器的 RTTI 实现。

---

## 新手要点（和 C 的区别）

- **C 没有 `typeid`**（C 的类型系统是编译期的，运行时没有类型信息）。C++ 的 RTTI 是运行时机制。
- **实用建议**：新手不需要装 Boost，用方法 1（故意编译错误）就够调试了。IDE（Cursor/VSCode）的鼠标悬停也能显示 `auto` 推导结果。
- **`typeid().name()` 不可信**：它退化了引用和 const。调试类型问题别只靠它。

---

## HFT 关联

- **调试万能引用推导**：模板转发 `template<class T> void f(T&& x)` 推出来的 `T` 是 `T` 还是 `T&` 是理解移动语义的关键——用方法 1 确认。
- **生产代码别留 typeid**：RTTI 有运行时开销（`typeid` 需要查 vtable），HFT 热路径不开 RTTI（`-fno-rtti`）。

---

## 自测题

1. 三种查看推导结果的方法分别是什么？各自有什么缺点？
2. `typeid(x).name()` 为什么不可靠？它丢失了什么信息？
3. 为什么生产 HFT 代码通常用 `-fno-rtti`？这对调试方式有何影响？
4. 方法 1（故意编译错误）的原理是什么？

---

## 参考与延伸

- 下一章：[第 2 章 auto](../ch02-auto/README.md)
- 回到：[第 1 章 类型推导](README.md)
