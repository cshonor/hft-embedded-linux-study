# Item 1：理解模板类型推导

> **所属章节：** [ch01 类型推导](./README.md) · Items 1–4 的第 1 条
> **难度：** ⭐⭐⭐（M1 硬门槛，第一次读不懂很正常，读完 ch04 智能指针再回来复习会顺很多）
> **一句话：** 模板推导有三种形态，按值传递会剥掉顶层 const。

---

## 为什么要学这个（先建立直觉）

如果你从 C 过来，函数参数的类型是你**手写死**的：

```c
// C：参数类型写死，传错类型编译器直接报错
void f(int x);        // 只能接 int
void g(const char* s); // 只能接 const char*
```

C++ 有了模板，编译器能**自动猜**参数类型，你不用写：

```cpp
template<class T>
void f(T param);   // T 是什么？编译器看你传什么就推什么
f(42);             // T = int
f(3.14);           // T = double
```

听起来很美好。**坑在于：** `T` 推成什么，不只取决于你传什么，还取决于形参 `param` 的写法（`T&`？`T&&`？`T`？）。同一个实参，形参换个写法，`T` 就不一样。而且推导规则里有几个"反直觉"的地方（比如按值传参会把 const 剥掉），不懂规则就会在泛型代码里踩坑。

这节就是把这些规则讲清楚。**读不懂 muduo 回调、DPDK C++ 封装里的 `template<class F> void set_cb(F&& f)`，根因多半是这节没吃透。**

---

## 这节讲什么

模板形如 `template<class T> void f(ParamType param)`，调用 `f(expr)` 时编译器干两件事：
1. 根据 `expr` 推导 `T` 的类型
2. 根据 `ParamType` 的形态（引用？万能引用？按值？）调整最终类型

关键：**`T` 的推导结果不仅取决于 `expr`，还取决于 `ParamType` 长什么样。** 下面三种形态分别讲。

---

## 形态一：ParamType 是引用（`T&`）

**一句话直觉：** 形参是引用时，实参的"引用性"被剥掉，const 被保留进 T。

```cpp
template<class T>
void f(T& param);

int x = 27;
const int cx = x;
const int& rx = x;   // rx 是 x 的 const 引用

f(x);    // T 推为 int，        param 类型是 int&
f(cx);   // T 推为 const int，  param 类型是 const int&
f(rx);   // T 推为 const int，  param 类型是 const int&（引用性被忽略）
```

**规则：** `expr` 的引用性被忽略，`const` 被保留进 `T`。

**新手陷阱：** `rx` 虽然是 `const int&`，但推导时**引用性被剥掉**，`T` 推成 `const int`（不是 `const int&`）。`param` 的那个 `&` 是 `ParamType` 自己加的，不是从 `expr` 继承的。

---

## 形态二：ParamType 是万能引用（`T&&`）

> **术语铺垫：什么是"万能引用"？**
> 先别被名字吓到。普通右值引用 `void f(int&&)` 只能接右值（临时值）。但在模板里 `template<class T> void f(T&&)`，这个 `T&&` 是个"变色龙"——传左值它变左值引用，传右值它变右值引用，所以叫"万能引用"（universal reference）。
> **判断标准：** 只有当**类型推导发生**（模板 + `T&&`）时才是万能引用。`void f(int&&)` 这种没有推导的，就是普通右值引用。
> 这玩意儿是完美转发的根基（Item 25 详谈），现在先记住"能接左右两种值"就行。

```cpp
template<class T>
void f(T&& param);

int x = 27;
f(x);    // x 是左值 → T 推为 int&，  param 是 int& && → 折叠成 int&
f(27);   // 27 是右值 → T 推为 int，  param 是 int&&
```

**规则：**
- 左值实参 → `T` 推为 `T&`（引用折叠后 `param` 是 `T&`）
- 右值实参 → `T` 推为 `T`（`param` 是 `T&&`）

> **术语铺垫：什么是"引用折叠"？**
> C++ 不允许"引用的引用"（`int& &`），但模板推导会产生这种情况。规则很简单：**只要其中有一个是左值引用（`&`），结果就是 `&`；只有两个都是右值引用（`&&`），结果才是 `&&`。**
> 记法：`&` 比 `&&` "强"，一个 `&` 就把 `&&` 拉下水。
> 这规则决定了万能引用接左值时变成左值引用——不是魔法，是折叠。

---

## 形态三：ParamType 是按值（`T`）

**一句话直觉：** 按值传递 = 拷贝一份，副本上的 const 没意义，所以顶层 const 被剥掉。

```cpp
template<class T>
void f(T param);  // 按值传递（拷贝）

int x = 27;
const int cx = x;
const char* const ptr = "hello";  // 顶层 const（指针本身）+ 底层 const（指向的字符）

f(x);    // T 推为 int
f(cx);   // T 推为 int（顶层 const 被剥掉！）
f(ptr);  // T 推为 const char*（顶层 const 剥掉，底层 const 保留）
```

**规则：** 按值传递会剥掉**顶层 const** 和引用性，但保留**底层 const**。

**新手陷阱：** 这是最容易踩的坑——传 `const int` 给按值形参，`T` 是 `int` 不是 `const int`。很多人在泛型代码里发现"const 怎么没了"，根因在这。

**顶层 vs 底层 const 速记：**
| 表达式 | 顶层 const | 底层 const | 大白话 |
|--------|-----------|-----------|--------|
| `const int x` | ✅（x 本身不可变） | — | 变量本身不能改 |
| `const int* p` | ❌（p 可变） | ✅（指向的 int 不可变） | 能换指向，不能改内容 |
| `int* const p` | ✅（p 本身不可变） | ❌ | 不能换指向，能改内容 |
| `const int* const p` | ✅ | ✅ | 都不能动 |

> **记法：** "顶层"= 最外层那个 const（修饰变量/指针本身），"底层"= 钻进去修饰所指对象的 const。按值传参剥的是顶层（因为副本本身重新生成，const 无意义），底层保留（因为还指着原来的 const 数据）。

---

## 数组/函数实参的退化

和 C 完全一样的规则：除非 `ParamType` 是引用，否则数组退化为指针、函数退化为函数指针：

```cpp
const char name[] = "hello";  // const char[6]

template<class T> void f(T param);
f(name);  // T 推为 const char*（数组退化）

template<class T> void f(T& param);
f(name);  // T 推为 const char (&)[6]（引用不退化）
```

**应用：** 用引用形参可以写出"编译期推导数组长度"的模板（C 做不到）：

```cpp
template<class T, size_t N>
constexpr size_t arraySize(T (&)[N]) noexcept { return N; }

int keyMap[256];
std::array<int, arraySize(keyMap)> mappedKeys;  // 编译期拿 256，零运行开销
```

---

## 完整可运行例子

把三种形态放一起，编译跑一下看输出（g++ -std=c++17）：

```cpp
#include <iostream>
#include <typeinfo>

template<class T> void byRef(T& param)       { std::cout << "byRef    T=" << typeid(T).name() << "\n"; }
template<class T> byUniRef(T&& param)        { std::cout << "uniRef   T=" << typeid(T).name() << "\n"; }
template<class T> void byVal(T param)        { std::cout << "byVal    T=" << typeid(T).name() << "\n"; }

int main() {
    int x = 27;
    const int cx = x;
    const int& rx = x;

    byRef(x);    // T = int
    byRef(cx);   // T = const int      ← const 保留
    byRef(rx);   // T = const int      ← 引用性剥掉

    byUniRef(x); // T = int&           ← 左值，万能引用变左值引用
    byUniRef(27);// T = int            ← 右值

    byVal(x);    // T = int
    byVal(cx);   // T = int            ← 顶层 const 剥掉！
}
```

> 注：`typeid(T).name()` 输出是编译器相关的 mangled 名（gcc 下 `int` 是 `i`，`const int` 是 `Ki`），重点是看**不同形态下 T 不同**，不用纠结名字。

---

## 常见错误（新手踩坑）

**错误 1：以为 `T&&` 就是右值引用**
```cpp
template<class T> void f(T&& x);  // 这是万能引用，不是右值引用！
void g(int&& x);                  // 这才是纯右值引用（没有推导）
```
**为什么错：** 混淆了"有推导的 `T&&`"和"写死的 `int&&`"。前者是万能引用，后者才是右值引用。

**错误 2：按值传参后以为还能靠 const 保护副本**
```cpp
template<class T> void f(T param) { param = 10; }  // 能编译！
const int cx = 5;
f(cx);  // T = int（const 被剥），param 能被改
```
**为什么没事但有坑：** 改的是副本，不影响 `cx`。但如果你以为 `T` 是 `const int` 而依赖它（比如拿 `T` 做别的推导），就会出错。

**错误 3：想从 `T` 反推实参有没有 const**
按值传递剥 const，按引用保留 const——所以 `T` 带不带 const 取决于 `ParamType`，不能反推实参。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 C++ 这样 |
|------|---------|-----------|----------------|
| 参数类型 | 手写死 | 模板自动推导 | 泛型编程，一份代码多类型复用 |
| 引用 | 没有，全按值/指针 | 有 `T&`，推导时剥引用性 | 引用是别名，推导看的是"原始对象" |
| const 传递 | 按值传 const 无意义（同 C++） | 按值剥顶层 const，按引用保留 | 副本重新生成，const 无意义；引用是别名，const 有意义 |
| 万能引用 | 无（C 没有右值引用） | 模板 `T&&` 能接左右值 | 为了完美转发，一个接口接所有 |
| 数组退化 | 退化为指针 | 按值退化，按引用不退化 | 和 C 一致，引用是 C++ 新增的例外 |

**一句话总结：** C 程序员记住三点——① C 没有引用，`T&` 是别名不是指针；② 按值传参会剥 const（和 C 一样，但模板里容易忘）；③ `T&&` 在模板里是万能引用不是右值引用（C 根本没这个概念）。

---

## HFT 关联

- **万能引用 + 完美转发**是 muduo / DPDK C++ 封装里回调注册的基石：`template<class F> void set_cb(F&& f)` 用万能引用避免不必要的拷贝。不理解第三种形态，读不懂这类接口。
- **按值传参剥 const** 影响 HFT 里"不可变配置"的传递：如果用模板按值传 `const Config`，`T` 是 `Config` 不是 `const Config`，函数内能改副本——这通常没问题，但要意识到。
- **数组长度推导**：HFT 里固定大小缓冲区（如行情快照数组）可以用 `arraySize` 模板在编译期拿长度，零运行开销。

---

## 自测题

**概念题：**
1. `template<class T> void f(T param);` 调用 `f(cx)`（`cx` 是 `const int`），`T` 推导成什么？为什么顶层 const 消失？
2. `template<class T> void f(T&& param);` 分别调用 `f(x)`（左值）和 `f(27)`（右值），`T` 和 `param` 的类型分别是什么？
3. `template<class T> void f(T& param);` 调用 `f(rx)`（`rx` 是 `const int&`），`T` 推导成什么？引用性为什么被忽略？
4. `const char* const p` 传给按值形参 `T param`，`T` 是什么？顶层和底层 const 分别怎么处理？

**踩坑题：**
5. 下面代码能编译吗？`T` 是什么？`param` 能被赋值吗？
```cpp
template<class T> void f(T param) { param = 10; }
const int cx = 5;
f(cx);
```
6. 下面两个 `f`，哪个是万能引用？为什么？
```cpp
template<class T> void f(T&& x);
void g(int&& x);
```
7. 写一个模板函数，能在编译期推导出 C 风格数组的长度（提示：用引用形参防止退化）。

---

## 参考与延伸

- **本书关联：** Item 2（auto 推导几乎等同模板，但有个例外）、Item 24（万能引用 vs 右值引用怎么区分）、Item 25（`move` vs `forward` 的使用场景）
- **C 语言对照：** 《C 和指针》ch08——数组退化为指针的规则，C++ 完全继承
- **下一节：** [Item 2：理解 auto 类型推导](./item02-auto-type-deduction.md)
