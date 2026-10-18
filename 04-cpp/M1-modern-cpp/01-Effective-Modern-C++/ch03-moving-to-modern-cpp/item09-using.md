# Item 9：优先 using 别名而非 typedef

> 第 3 章 移步现代 C++ · Item 9 · 上一节：[Item 8 nullptr](item08-nullptr.md)

## 为什么要学这个（先建立直觉）

C 程序员对 `typedef` 再熟不过：`typedef unsigned long ulong;`、`typedef void (*Callback)(int);`——给长类型起短名，天经地义。到了 C++，这套东西还能用，但会遇到一面墙：**模板**。

假设你写了一个自定义分配器，想让代码里的 `vector` 默认走内存池。你要的不是一个别名，而是一**族**别名——`PoolVec<int>`、`PoolVec<Order>`……`typedef` 在这里直接缴械：C++ 语法里根本不存在"模板 typedef"这种东西，想模板化只能包一层 `struct` 再取 `::type`，用起来还得带 `typename`，又丑又绕。

C++11 的 `using` 把这面墙拆了：`template<class T> using PoolVec = std::vector<T, MyAlloc<T>>;` 一行搞定。顺手还解决了可读性问题——`using Callback = void(*)(int);` 从左往右读，名字就在等号左边，不像 `typedef void (*Callback)(int);` 把名字埋在星号括号堆里。

---

## 这节讲什么

1. `using` 与 `typedef` 的逐条对照（普通别名二者等价）；
2. **别名模板**：`using` 独有的能力，`typedef` 的 struct+`::type` 绕行方案（附实测报错）；
3. C++14 类型萃取 `_t` 后缀（`std::add_pointer_t` 等）的由来——它就是别名模板；
4. 全部论断本机实测：clang 23.1.0，arm64-apple-darwin，libc++。

---

## typedef vs using 逐条对照

| 维度 | `typedef` | `using` |
|------|-----------|---------|
| 普通别名 | `typedef unsigned long ulong;` | `using ulong = unsigned long;`（语义完全等价） |
| 函数指针 | `typedef void (*Cb)(int);` 名字埋中间 | `using Cb = void(*)(int);` 名字在左边 |
| 模板别名 | **做不到**（语法不存在） | `template<class T> using V = ...;` |
| 模板元编程 | 必须包 `struct` + `typename ...::type` | 直接实例化 |
| 依赖类型语境 | 要 `typename` | 同样要 `typename`（无差异） |
| 引入版本 | 从 C 继承 | C++11 |

普通别名上两者**编译产物完全相同**——选 `using` 不花一分钱，纯赚可读性和模板化能力。

---

## 普通别名：语法方向的差别

```cpp
#include <string>

typedef unsigned long ulong_td;
using ulong_using = unsigned long;

// 函数指针是分水岭
typedef void (*Callback_td)(int, const std::string&);   // 名字藏在 (*...) 里
using Callback_using = void(*)(int, const std::string&); // 名字在最左，类型在最右
```

`typedef` 的声明方向和 C 一致："类型 名字"，遇到函数指针、数组就拧巴——名字被类型包在中间，读的时候要拆括号。`using` 是赋值式语法：`新名 = 类型`，和 `auto x = expr;` 的阅读方向一致，读长类型时视线不用来回跳。

---

## 别名模板：using 独有的能力

### typedef 直接阵亡

```cpp
#include <vector>

template<typename T>
typedef std::vector<T> Vec;    // 想给 vector<T> 起模板别名

int main() { Vec<int> v; }
```

本机实测 clang 23.1.0 报错原文（`-std=c++17`）：

```
error: a typedef cannot be a template
    4 | typedef std::vector<T> Vec;
      |                        ^
error: no template named 'Vec'
```

没有商量的余地：typedef 在语法上就不接受模板参数。

### typedef 时代的绕行：struct + ::type

C++11 之前想达到同样效果，只能把 typedef 塞进 struct：

```cpp
template<typename T>
struct Vec_t {
    typedef std::vector<T, MyAlloc<T>> type;
};

Vec_t<int>::type v;                    // 非模板语境
typename Vec_t<T>::type w;             // 模板语境还要加 typename
```

两个负担：使用点要写 `::type`；在模板里使用还得加 `typename`（因为 `Vec_t<T>::type` 是依赖类型，编译器不知道它是类型还是静态成员）。

### using 一步到位

```cpp
template<typename T>
using Vec = std::vector<T, MyAlloc<T>>;

Vec<int> v1{1, 2, 3};            // 直接用，没有 ::type，没有 typename
Vec<std::string> v2;             // 每个实例化都自动带 MyAlloc
```

实测对比（clang 23.1.0，完整代码见下文"完整可运行例子"）：

```
v1.size()=3 v2.size()=3 v3.size()=1
```

`static_assert(std::is_same_v<Vec<int>, typename Vec_t<int>::type>);` 通过——两条路线产出**完全相同**的类型，差别全在书写和使用体验。

---

## C++14 的 _t 后缀：别名模板的最佳广告

C++11 的类型萃取用起来很啰嗦：

```cpp
typename std::remove_reference<T>::type   // C++11：三步走
```

`remove_reference<T>` 是模板，`::type` 取结果，`typename` 声明依赖类型——三个噪声一个都不能少。C++14 标准库给每个萃取都配了 `_t` 后缀的别名模板：

```cpp
template<class T>
using remove_reference_t = typename remove_reference<T>::type;  // 标准库里就这行
```

使用变成一行：

```cpp
std::remove_reference_t<T>        // C++14：干净
std::add_pointer_t<int> p = nullptr;   // p 是 int*
```

本机实测（clang 23.1.0）：

- `-std=c++11` 下 `std::add_pointer_t<int>` 报错：`error: no member named 'add_pointer_t' in namespace 'std'`
- `-std=c++14` 起编译通过，`static_assert(std::is_same_v<decltype(p), int*>)` 成立。

`_t` 后缀的本质就是别名模板——C++11 有 `using` 之后，标准库才有条件把这些便利别名批量铺出来。**你自己写元编程时也该照办：每个 `struct Foo { typedef ... type; };` 旁边跟一个 `template<class T> using Foo_t = typename Foo<T>::type;`。**

---

## 完整可运行例子

```cpp
#include <vector>
#include <memory>
#include <iostream>
#include <type_traits>

// 合法的自定义分配器（满足 rebind 要求，能过 libc++ 的 static_assert）
template<typename T>
struct MyAlloc {
    using value_type = T;
    MyAlloc() = default;
    template<typename U> MyAlloc(const MyAlloc<U>&) {}
    T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
    void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
};
template<typename T, typename U>
bool operator==(const MyAlloc<T>&, const MyAlloc<U>&) { return true; }
template<typename T, typename U>
bool operator!=(const MyAlloc<T>&, const MyAlloc<U>&) { return false; }

// typedef 路线：包 struct + ::type
template<typename T>
struct Vec_t { typedef std::vector<T, MyAlloc<T>> type; };

// using 路线：别名模板一步到位
template<typename T>
using Vec = std::vector<T, MyAlloc<T>>;

typedef void (*FP_td)(int, double);      // 名字藏在中间
using FP_using = void(*)(int, double);   // 名字在左边

void handler(int, double) {}

int main() {
    Vec<int> v1{1, 2, 3};                  // using 直接用
    typename Vec_t<int>::type v2{1, 2, 3}; // typedef 要带 ::type
    Vec_t<double>::type v3;
    v3.push_back(1.5);

    FP_td f1 = handler;
    FP_using f2 = handler;
    f1(1, 2.0); f2(1, 2.0);

    static_assert(std::is_same_v<Vec<int>, typename Vec_t<int>::type>);
    std::cout << "v1.size()=" << v1.size()
              << " v2.size()=" << v2.size()
              << " v3.size()=" << v3.size() << "\n";

    std::add_pointer_t<int> p = nullptr;   // C++14 的 _t 后缀
    static_assert(std::is_same_v<decltype(p), int*>);
    std::cout << "add_pointer_t<int> is int*: ok\n";
}
```

本机实测 clang 23.1.0（`-std=c++17 -Wall`）输出：

```
v1.size()=3 v2.size()=3 v3.size()=1
add_pointer_t<int> is int*: ok
```

一个顺手的坑：自定义分配器如果直接 `struct MyAlloc : std::allocator<T> {}`，libc++ 实测会拒绝——`[allocator.requirements] states that rebinding an allocator to the same type should result in the original allocator`。继承来的 `rebind` 给出的是 `std::allocator<U>` 而非 `MyAlloc<U>`，违反 rebind 必须回到自身的要求。正确姿势如上面的独立实现（或显式提供 `template<class U> struct rebind`）。

---

## 常见错误（新手踩坑）

1. **试图写模板 typedef**。`template<class T> typedef std::vector<T> Vec;` 实测报 `error: a typedef cannot be a template`。修正：换 `using`。
2. **typedef 绕行方案忘了 `typename`**。模板里写 `Vec_t<T>::type v;` 报依赖类型错误。修正：`typename Vec_t<T>::type v;`——或者干脆别绕行，用 `using`。
3. **C++11 项目里用 `_t` 后缀**。`std::add_pointer_t` 等是 C++14 才有的，实测 `-std=c++11` 报 `no member named 'add_pointer_t'`。修正：C++11 下自己补一行 `template<class T> using add_pointer_t = typename std::add_pointer<T>::type;`。
4. **以为 `using` 和 `typedef` 生成的类型有差别**。没有——别名不产生新类型，`Vec<int>` 和 `std::vector<int, MyAlloc<int>>` 是同一个类型，`static_assert(is_same_v<...>)` 实测通过。要"真·新类型"得用 `enum class`、强类型包装 struct 等别的技术。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 类型别名 | `typedef` | `using` | 语法直观，支持模板 |
| 模板别名 | 不适用（C 无模板） | `template<class T> using X = ...` | `typedef` 语法上做不到 |
| 函数指针别名 | `typedef void (*F)(int);` | `using F = void(*)(int);` | 名字在左边，一眼可读 |

**一句话总结：** C 程序员把 `typedef` 全部换成 `using` 就行，语法是 `using 新名 = 原类型;`，从左到右读，和 `auto` 一样自然——还白捡模板化能力。

---

## HFT 关联

- **内存池容器别名**：`template<class T> using PoolVec = std::vector<T, PoolAlloc<T>>;` 让"所有容器走对象池"这条纪律变成一行别名，策略代码里 `PoolVec<Level>` 比 `std::vector<Level, PoolAlloc<Level>>` 少一个数量级的视觉噪声。
- **回调签名集中管理**：`using TickCallback = void(*)(const Tick&, SeqNo);` 改签名只改一处；`typedef` 写法在函数指针上可读性差，评审时容易漏看参数。
- **编译期计算的 `_t` 风格**：低延迟代码常用类型萃取做编译期分派（`if constexpr (std::is_trivially_copyable_v<T>)`），自己写萃取时顺手提供 `_t`/`_v` 别名，调用点才不会被 `typename ...::type` 淹没。
- **ABI 注意**：别名不改类型，`PoolVec<int>` 的 mangled name 仍含分配器类型——跨库边界改分配器就是改 ABI，别名让这件事**看起来**没变，部署时要清醒。

---

## 自测题

**1. `using X = Y;` 和 `typedef Y X;` 生成的类型有差别吗？**
<details><summary>答案</summary>
没有，语义完全等价，都是给既有类型起别名、不产生新类型。实测 `static_assert(std::is_same_v<Vec<int>, typename Vec_t<int>::type>)` 通过。差别只在可读性和模板化能力。
</details>

**2. 为什么 `typedef` 不能定义别名模板？**
<details><summary>答案</summary>
`typedef` 的语法是"为已确定的类型起名"，没有放置模板参数列表的位置。实测 `template<class T> typedef std::vector<T> Vec;` 报 `error: a typedef cannot be a template`。`using` 的"名字 = 类型"语法天然可以在前面加 `template<...>` 形成别名模板。
</details>

**3. typedef 想达到别名模板的效果怎么办？代价是什么？**
<details><summary>答案</summary>
包一层 struct：`template<class T> struct Vec_t { typedef ... type; };`。代价：使用点要写 `::type`，模板语境还要加 `typename`（`typename Vec_t<T>::type`），可读性和书写都更繁琐。
</details>

**4. `using Callback = void(*)(int);` 比 `typedef void (*Callback)(int);` 好在哪？**
<details><summary>答案</summary>
可读性：`using` 名字在最左、类型在最右，与 `auto x = expr` 同向；`typedef` 把名字埋进 `(*...)` 里，函数签名越复杂越难拆。二者生成的类型完全相同。
</details>

**5. C++14 的 `std::remove_reference_t` 是怎么来的？C++11 下怎么办？**
<details><summary>答案</summary>
它就是标准库里的一行别名模板：`template<class T> using remove_reference_t = typename remove_reference<T>::type;`。C++11 没有这些 `_t` 别名（实测 `add_pointer_t` 报 `no member named`），需要自己用 `using` 补定义。
</details>

**6. 模板里 `using VT = Container::value_type;` 能直接写吗？**
<details><summary>答案</summary>
不能。`Container::value_type` 是依赖类型，必须写 `using VT = typename Container::value_type;`。`using` 不豁免 `typename`——依赖类型的 `typename` 要求与 `typedef` 相同（C++20 起部分语境可省略）。
</details>

---

## 参考与延伸

- 上一节：[Item 8 nullptr](item08-nullptr.md)
- 下一节：[Item 10 scoped enum](item10-scoped-enum.md)
- 相关：[Item 2 auto 类型推导](../ch01-deducing-types/item02-auto-type-deduction.md)（`using` 与 `auto` 同向的赋值式语法）
- 回到：[第 3 章 移步现代 C++](README.md)
