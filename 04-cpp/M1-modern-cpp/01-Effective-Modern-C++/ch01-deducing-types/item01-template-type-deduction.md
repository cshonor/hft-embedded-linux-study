# Item 1：理解模板类型推导

> **所属章节：** [ch01 类型推导](./README.md) · Items 1–4 的第 1 条
> **难度：** ⭐⭐⭐（M1 硬门槛，第一次读不懂很正常，读完 ch04 智能指针再回来复习会顺很多）
> **一句话：** 模板推导有三种形态，按值传递会剥掉顶层 const；文末深挖节讲清"推导为什么看不见 `&`"的机制根源。

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

关键：**`T` 的推导结果不仅取决于 `expr`，还取决于 `ParamType` 长什么样。** 下面三种形态分别讲，最后一节深挖"为什么推导看不见 `&`"——语言层与实现层的分层，以及四种常见说法的勘误。

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

> 机制根源见下面「深挖：为什么推导看不见 `&`」——这不是一条孤立的记忆规则，而是"表达式没有引用类型"的必然结果。

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

## 深挖：为什么推导看不见 `&`——语言层与实现层的分层

先把三种形态摆在一起看同一个 `rx`：

```cpp
template<class T> void byRef(T& param);     // 形态一
template<class T> void byUniRef(T&& param); // 形态二
template<class T> void byVal(T param);      // 形态三

int x = 27;
const int& rx = x;

byRef(rx);    // T = const int
byUniRef(rx); // T = const int&   ← T 自己带上了 &
byVal(rx);    // T = int
```

形态二那一行是分水岭：**`T` 本身完全可以被推成引用类型**。所以"模板推导永远看不到引用"是错的，正确的说法是"模板推导看到的是**实参表达式**的类型，而表达式层面从来没有引用"。这一节把这句话拆成标准条款、编译器实现、可验证证据三层。

### 第一层：语言标准——表达式没有引用类型

C++ 标准 [expr.type]/1 规定（逐字）：

> If an expression initially has the type "reference to T", the type is adjusted to T prior to any further analysis.

即 `rx` 这个**名字**的声明类型是 `const int&`，但一旦把它当表达式用（比如做实参），类型先被调整为 `const int`，引用属性当场消失。形态一 `T = const int` 的全部机制就在这一句里——不是"被剥掉"，是"从来没有"。

同一条规则也解释了为什么引用用起来像指针：

| 写法 | 结果 | 机制 |
|---|---|---|
| `sizeof(ra)` | 等于 `sizeof(int)` | `sizeof` 作用到被引用对象上 |
| `&ra` | 类型 `int*`，值是 `a` 的地址 | 取到的是被引用对象的地址 |
| `int&* p = &ra;` | 编译错误 | 引用不是对象，没有"引用的地址" |
| `int& arr[2];` | 编译错误 | 引用不是对象，做不了数组元素 |

gcc 13.2 对后两条的报错原文（CE 实测，逐字）：

```
error: cannot declare pointer to 'int&'
error: declaration of 'arr' as array of references
```

标准里还有一条常被忽略的规定，[dcl.ref]/4（逐字）：

> It is unspecified whether or not a reference requires storage.

**引用占不占内存，标准根本不管。** 所以"引用是别名、没有自己的存储"不是标准承诺，而是语言模型层的描述；实现层怎么做，看下一段。

### 第二层：编译器实现——只在"需要被存储"时才退化成地址

网传说法"引用底层用指针实现、每次用都自动解引用"只在引用**需要占据存储或跨边界传递**时成立。CE 实测（gcc 13.2，x86-64，`-O2`），同一个逻辑的两种签名，汇编逐字相同：

```cpp
int f_ref(int& r) { return r; }
int f_ptr(int* p) { return *p; }
```

```
f_ref(int&):
        mov     eax, DWORD PTR [rdi]
        ret
f_ptr(int*):
        mov     eax, DWORD PTR [rdi]
        ret
```

但编译器经常连这个地址都不留。CE 实测（gcc 13.2，`-O0 -Wall -Wextra`，build code 0、0 诊断，全部 `static_assert` 通过）：

```cpp
#include <cstdio>
struct WithRef { int& r; };    // 成员引用：必须存地址

int main() {
    int a = 10;
    int& ra = a;
    static_assert(sizeof(ra) == sizeof(int));
    printf("sizeof(ra)=%zu  sizeof(int)=%zu\n", sizeof(ra), sizeof(int));
    printf("&ra == &a : %d\n", (void*)&ra == (void*)&a);
    printf("sizeof(WithRef)=%zu  alignof(WithRef)=%zu\n",
           sizeof(WithRef), alignof(WithRef));
}
```

输出：

```
sizeof(ra)=4  sizeof(int)=4
&ra == &a : 1
sizeof(WithRef)=8  alignof(WithRef)=8
```

读法：**普通引用 `ra` 报的是被引用对象的大小（4），成员引用 `WithRef` 才真占一个指针槽位（8）**。函数内能内联的引用在 `-O2` 下常被整体消掉——既不存地址，也不解引用。因此"底层是指针"是**实现观察**，不是**推理前提**；拿汇编层面的经验去推模板推导的结果，必然出错。

### 第三层：模板推导读的是哪个类型

推导规则在 [temp.deduct.call]/3（逐字）：

> If P is a reference type, the type referred to by P is used for type deduction.

翻译成三种形态：

| ParamType | 规则 | `f(rx)` 的 T |
|---|---|---|
| `T&` | 用被引用类型推导，cv 保留 | `const int` |
| `T&&` | 左值实参 → T 推成左值引用，再折叠 | `const int&` |
| `T` | 剥顶层 const、剥引用 | `int` |

第二行和第一行的差别不在"剥不剥引用"，而在 **`T&&` 的推导规则本身就会产生引用**（左值实参 → `T` = `cv T&`，再经引用折叠得到 `param` 的最终类型）。CE 实测（gcc 13.2，`static_assert` 全部通过）：

```cpp
#include <type_traits>
template<class T> struct As { using type = T; };
template<class T> As<T> probeRef(T&);
template<class T> As<T> probeUni(T&&);
template<class T> As<T> probeVal(T);

int x = 27;
const int cx = x;
const int& rx = x;

static_assert(std::is_same_v<decltype(probeRef(rx)), As<const int>>);
static_assert(std::is_same_v<decltype(probeUni(rx)), As<const int&>>);  // T 带 &
static_assert(std::is_same_v<decltype(probeVal(cx)), As<int>>);
```

### 为什么 `decltype` 看得见 `&`，模板推导看不见

两者读的不是同一个东西：

| 工具 | 输入 | 遇到 `const int& rx` |
|---|---|---|
| 模板推导 | 实参**表达式**的类型（已被 [expr.type]/1 调整） | 收到 `const int`，`&` 已不存在 |
| `decltype(rx)` | 语法形式：无括号 id-expression | 直读实体声明类型 → `const int&` |
| `decltype((rx))` | 语法形式：括号 → 表达式 | 走值类别分支，左值 → `T&` → `const int&` |

一句话：**`decltype` 是唯一能复原引用类型的工具，因为它绕过了表达式类型调整这一步**；模板推导不可能看见 `&`，因为 `&` 在进入推导之前就没了。`decltype` 的两条分支规则与加括号陷阱，在 [Item 3：理解 decltype](item03-decltype.md) 展开。

### 四种常见说法的勘误

| 常见说法 | 判定 | 问题 |
|---|---|---|
| 模板推导会剥掉表达式的引用 | 半对 | 引用在表达式层面本就不存在，谈不上"剥"；且 `T&&` + 左值时 `T` 被推成 `const int&`，说"永远剥掉"是错的 |
| 引用底层存地址，每次用都自动解引用 | 半对 | 只在需要存储/跨边界时成立；`-O2` 下常整体消掉，成员引用才真占 8 字节 |
| `decltype(rx)` 与 `decltype((rx))` 结果不同 | 条件成立 | `rx` 是 `const int&` 时两者都得 `const int&`（巧合），必须用非引用变量才看得出差别 |
| 表达式 `ra` 是独立对象 | 错 | 引用不是对象：`sizeof(ra)==sizeof(int)`、`&ra` 得 `int*`、`int&*` 与引用数组直接编译错误 |

前两条管"语言层 vs 实现层"，后两条管"记特例 vs 记机制"——这条笔记的价值就在于把两组分清。

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

**错误 4：以为"引用底层是指针封装"，所以 `rx` 的类型应该是指针**
```cpp
const int& rx = x;
byRef(rx);   // 有人据此猜 T = const int* —— 错，T = const int
```
**为什么错：** 混淆了语言层和实现层。底层汇编里引用常用指针实现，但模板推导读的是 C++ 表达式的类型，而标准 [expr.type]/1 规定表达式中的引用先被调整为被引用类型。用汇编层面的经验推模板结果，是这条错误观念的根源。详见「深挖：为什么推导看不见 `&`」。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 C++ 这样 |
|------|---------|-----------|----------------|
| 参数类型 | 手写死 | 模板自动推导 | 泛型编程，一份代码多类型复用 |
| 引用 | 没有，全按值/指针 | 有 `T&`，推导时引用性消失 | 引用是别名；标准 [expr.type]/1 规定表达式中的引用先调整为被引用类型，推导看到的是"原始对象" |
| 引用的存储 | 指针变量一定占内存 | 引用不保证占存储（[dcl.ref]/4 原话：unspecified） | 别名不需要独立存储；只有成员引用、跨边界传参等场景才退化成地址 |
| const 传递 | 按值传 const 无意义（同 C++） | 按值剥顶层 const，按引用保留 | 副本重新生成，const 无意义；引用是别名，const 有意义 |
| 万能引用 | 无（C 没有右值引用） | 模板 `T&&` 能接左右值 | 为了完美转发，一个接口接所有 |
| 数组退化 | 退化为指针 | 按值退化，按引用不退化 | 和 C 一致，引用是 C++ 新增的例外 |

**一句话总结：** C 程序员记住三点——① C 没有引用，`T&` 是别名不是指针；② 按值传参会剥 const（和 C 一样，但模板里容易忘）；③ `T&&` 在模板里是万能引用不是右值引用（C 根本没这个概念）。

---

## HFT 关联

- **万能引用 + 完美转发**是 muduo / DPDK C++ 封装里回调注册的基石：`template<class F> void set_cb(F&& f)` 用万能引用避免不必要的拷贝。不理解第三种形态，读不懂这类接口。
- **按值传参剥 const** 影响 HFT 里"不可变配置"的传递：如果用模板按值传 `const Config`，`T` 是 `Config` 不是 `const Config`，函数内能改副本——这通常没问题，但要意识到。
- **数组长度推导**：HFT 里固定大小缓冲区（如行情快照数组）可以用 `arraySize` 模板在编译期拿长度，零运行开销。
- **引用在 ABI 上像指针，在类型系统里不是**：`struct OrderBook { Tick& top_; }` 这类引用成员真占一个指针槽位（CE 实测 `sizeof` 为 8），"按引用传参不拷贝"在热路径上成立；但泛型代码里不能按指针的直觉推 `T`（`byRef(rx)` 得 `const int` 而不是指针）。写模板转发时以 [expr.type]/1 为准，算内存布局时以 `sizeof` 实测为准——两件事分开做，别用一层推另一层。

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

**概念深化题（语言层 vs 实现层）：**
8. `int a = 10; int& ra = a;` 表达式 `ra` 是独立对象吗？它的类型是什么？给出两条可以自己验证的判据。
9. `template<class T> void f(T&& param); const int& rx = x; f(rx);` 的 `T` 是什么？同样是 `rx`，为什么 `T&` 版本推出 `const int`、`T&&` 版本推出 `const int&`？
10. 有人说"引用底层是指针封装，所以 `byRef(rx)` 的 `T` 应该是指针"。这句话错在哪一步？用标准条款说明。

<details>
<summary>参考答案</summary>

1. `T` 推导为 `int`。按值形参（`ParamType` 就是 `T`）的推导规则是：先忽略实参的引用性，再忽略顶层 `const`/`volatile`。因为 `param` 是实参的一份独立拷贝，拷贝的 const 属性与实参无关，标准规定此时 `T` 不带顶层 const，所以 `const` "消失"了。

2. 左值 `f(x)`：`T` 推导为 `int&`，`param` 的类型 `int& &&` 经引用折叠变成 `int&`。右值 `f(27)`：`T` 推导为 `int`，`param` 类型是 `int&&`。这正是万能引用（universal reference）的专属规则：形参写成 `T&&` 且 `T` 需要推导时，左值实参会把 `T` 推成左值引用类型。

3. `T` 推导为 `const int`，`param` 的类型是 `const int&`。引用性被忽略是因为形参已经声明成引用，推导只关心"实参所引用对象的类型"；而 const 必须保留，否则就能通过 `param` 修改一个原本是 const 的对象，破坏 const 正确性。

4. `T` 是 `const char*`（指向 `const char` 的指针）。顶层 const（`p` 自己不可改指向）被丢弃；底层 const（指向的字符不可改）被保留。因此 `param` 可以重新赋值指向别的字符串，但不能通过它修改字符内容。

5. 能编译，`T` = `int`。`param` 是 `cx` 的一份 `int` 拷贝，`param = 10` 修改的是副本，`cx` 仍是 5。这也说明"按值传参会丢 const"是安全的——它没有绕过原对象的 const。

6. `template<class T> void f(T&& x)` 是万能引用（universal reference / forwarding reference），因为它的形参形式恰好是 `T&&` 且 `T` 在该函数模板中被推导。`void g(int&& x)` 是普通右值引用：`int` 是具体类型、不涉及类型推导，只能绑定右值。判断口诀：形如 `T&&` 且 `T` 需要推导 → 万能引用；否则 → 右值引用。

7. 关键是形参必须写成数组引用，阻止数组退化成指针：
```cpp
template<class T, std::size_t N>
constexpr std::size_t arraySize(T (&)[N]) noexcept { return N; }

int a[10];
static_assert(arraySize(a) == 10);
```
`T (&)[N]` 让 `N` 在编译期被推导出来，配合 `constexpr` 可作为编译期常量使用，零运行开销。

8. 表达式 `ra` **不是独立对象**——引用不是对象（对象模型里没有引用的位置）。它的 value category 是左值，**类型是 `int`**（不是 `int&`）：[expr.type]/1 规定表达式中的引用先被调整为被引用类型。两条判据：`static_assert(sizeof(ra) == sizeof(int))`（CE 实测通过，输出 4 == 4）；`&ra` 的类型是 `int*` 且 `&ra == &a` 为真（CE 实测输出 1）。反向判据：`int&* p = &ra;` 与 `int& arr[2];` 都是编译错误（gcc 13.2 原文 `error: cannot declare pointer to 'int&'`、`error: declaration of 'arr' as array of references`）。

9. `T` 推出 `const int&`。`T&` 版本走 [temp.deduct.call]/3 的引用规则：用被引用类型推导、保留 cv，得到 `T = const int`，`&` 由 `ParamType` 提供。`T&&` 版本不同——`rx` 是左值，按万能引用规则 `T` 被推成**左值引用类型** `const int&`，再经引用折叠得到 `param` 的 `const int&`。即：`T&` 情形下 `&` 来自形参写法，`T&&` 情形下 `T` 自己就是引用。CE 实测三条 `static_assert` 全部通过（`probeRef(rx)` → `As<const int>`、`probeUni(rx)` → `As<const int&>`、`probeVal(cx)` → `As<int>`）。

10. 错在把**实现层**的事实当成**语言层**的规则。汇编里引用常用指针实现（`-O2` 下 `int f(int&)` 与 `int f(int*)` 汇编逐字相同，CE 实测），但模板推导读的是实参表达式的类型；[expr.type]/1 明确表达式中的引用先被调整为被引用类型，推导根本看不到指针。且"底层是指针"本身也不总成立——`-O2` 下局部引用常被整体消掉，成员引用才真占 8 字节（`sizeof(WithRef)==8`，CE 实测）。

</details>

---

## 参考与延伸

- **本书关联：** Item 2（auto 推导几乎等同模板，但有个例外）、Item 3（decltype 两条分支：为什么它看得见 `&` 而模板推导看不见）、Item 24（万能引用 vs 右值引用怎么区分）、Item 25（`move` vs `forward` 的使用场景）
- **标准条款：** [expr.type]/1（表达式中的引用先调整为被引用类型）、[dcl.ref]/4（引用是否占存储未作规定）、[temp.deduct.call]/3（P 为引用类型时的推导规则）
- **C 语言对照：** 《C 和指针》ch08——数组退化为指针的规则，C++ 完全继承
- **下一节：** [Item 2：理解 auto 类型推导](./item02-auto-type-deduction.md)
