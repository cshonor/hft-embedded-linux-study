# Item 17：理解特殊成员函数的生成规则

> 第 3 章 移步现代 C++ · Item 17 · 上一节：[Item 16 const 线程安全](item16-const-thread-safety.md)

## 为什么要学这个（先建立直觉）

C 的结构体只有值拷贝——`memcpy` 语义，编译器不需要生成任何"特殊函数"：

```c
struct Point { int x; int y; };
struct Point a = {1, 2};
struct Point b = a;    // 逐字节拷贝，没有"拷贝构造"的概念
```

C++ 的类有构造、析构、拷贝、移动——编译器会自动生成这些"特殊成员函数"。但生成规则很复杂，**声明了其中一个会影响其余的自动生成**。最常见的坑：你写了析构函数（比如要释放资源），编译器就**不再自动生成移动构造**，于是 `vector` 扩容退回拷贝（见 [Item 14](item14-noexcept.md)）——性能莫名变差，但代码能编译能运行。更阴的是：光看 `std::is_move_constructible` 你**发现不了**这个退化。

---

## 这节讲什么

1. 六个特殊成员函数都是谁，编译器什么时候生成它们；
2. 用 `static_assert` 打一张生成规则矩阵；
3. 用**计数器探针**戳穿 `is_move_constructible` 的假象——看 move 请求到底走没走 move；
4. 析构为什么会抑制移动（历史原因）；
5. Rule of Zero / Rule of Five，以及 `=default` 写在哪里决定类是否 trivial。

---

## 六个特殊成员函数

| 特殊成员函数 | C 术语类比 |
|-------------|-----------|
| 默认构造 | （无——C 自动零初始化或未初始化） |
| 析构 | （无——C 靠程序员手动 `free`） |
| 拷贝构造 | `struct B = A;`（memcpy 语义） |
| 拷贝赋值 | `B = A;`（memcpy 语义） |
| 移动构造 | （无——C 没有移动语义） |
| 移动赋值 | （无） |

前四个 C++03 就有，后两个 C++11 引入。C++11 的"大三律"因此扩展为"大五律"。

---

## 生成规则矩阵（static_assert 实测）

定义一组类，每种只声明一种特殊成员函数，然后用 `is_copy_constructible` / `is_move_constructible` 等 trait 打表：

```cpp
#include <type_traits>

struct Empty {};
struct DtorOnly { ~DtorOnly() {} };
struct CopyCtorOnly { CopyCtorOnly() = default; CopyCtorOnly(const CopyCtorOnly&) {} };
struct MoveCtorOnly { MoveCtorOnly(MoveCtorOnly&&) {} };

// 用 static_assert 把规则钉死
static_assert( std::is_copy_constructible<Empty>::value, "");
static_assert( std::is_move_constructible<Empty>::value, "");
static_assert( std::is_copy_constructible<DtorOnly>::value, "");
static_assert( std::is_copy_constructible<CopyCtorOnly>::value, "");
static_assert(!std::is_copy_constructible<MoveCtorOnly>::value, "move 声明后 copy 被删除");
static_assert(!std::is_copy_assignable<MoveCtorOnly>::value,   "move 声明后 copy 赋值被删除");
static_assert( std::is_move_constructible<MoveCtorOnly>::value, "");
```

clang 23.1.0 实测（`-std=c++11/14/17/20` 四种标准下结果**完全一致**，全部 `static_assert` 通过）：

```
Empty         copyC=1  moveC=1  copyA=1  moveA=1  triviallyCopyable=1
DtorOnly      copyC=1  moveC=1  copyA=1  moveA=1  triviallyCopyable=0
CopyCtorOnly  copyC=1  moveC=1  copyA=1  moveA=1  triviallyCopyable=0
MoveCtorOnly  copyC=0  moveC=1  copyA=0  moveA=0  triviallyCopyable=0
```

注意 `moveC` 这一列：`DtorOnly` 和 `CopyCtorOnly` 都是 1（true），**但它们根本没有移动构造函数**。这个 trait 在这儿说谎了。

---

## 关键陷阱：is_move_constructible 会骗你

`std::is_move_constructible<T>` 问的是"能不能用 `T&&` 构造出 `T`"——而 `T&&` 完全可以绑到 `const T&` 上，去调拷贝构造。所以只要类可拷贝，这个 trait 就是 `true`，**跟有没有移动构造无关**。

```cpp
struct Probe {
    static int copies, moves;
    Probe() = default;
    Probe(const Probe&) { ++copies; }
    Probe(Probe&&) noexcept { ++moves; }
};
int Probe::copies = 0;
int Probe::moves  = 0;
```

让外层的类各自持有一个 `Probe p;`，然后发起 `W b = std::move(a);` 看计数器：

```cpp
struct W_Empty    { Probe p; };
struct W_Dtor     { Probe p; ~W_Dtor() {} };
struct W_CopyCtor { Probe p; W_CopyCtor() = default; W_CopyCtor(const W_CopyCtor&) = default; };
struct W_MoveCtor { Probe p; W_MoveCtor() = default; W_MoveCtor(W_MoveCtor&&) = default; };
```

clang 23.1.0（`-std=c++17 -Wall`）实测输出：

```
W_Empty      probe copies=0  moves=1  -> MOVE
             is_copy_constructible_v=true  is_move_constructible_v=true
W_Dtor       probe copies=1  moves=0  -> COPY(fallback)
             is_copy_constructible_v=true  is_move_constructible_v=true
W_CopyCtor   probe copies=1  moves=0  -> COPY(fallback)
             is_copy_constructible_v=true  is_move_constructible_v=true
W_MoveCtor   probe copies=0  moves=1  -> MOVE
             is_copy_constructible_v=false  is_move_constructible_v=true
```

| 类 | 声明了 | 编译器生成 | 结论 |
|---|-------|-----------|------|
| `W_Empty` | 无 | 全部 5 个 | 移动请求走 `Probe(Probe&&)` |
| `W_Dtor` | 析构 | 拷贝生成，**移动被抑制** | 移动请求退化成拷贝 |
| `W_CopyCtor` | 拷贝构造 | 移动被抑制 | 同上 |
| `W_MoveCtor` | 移动构造 | **拷贝被删除**，移动赋值也不生成 | 只能移动 |

`W_Dtor` 那行最扎心：**`is_move_constructible` 是 true，`copies` 却是 1**——编译器按"拷"给你办了。这就是 Item 17 真正的杀手锏：不显式 `=default` 移动操作，光有移动语义的成员也白搭。

---

## 为什么析构会抑制移动？

历史包袱。C++03 时代没有移动语义，写了析构函数说明类在手动管理资源，编译器按成员拷贝生成拷贝操作（指针成员会导致浅拷贝，那是另一回事）。

C++11 引入移动语义后，委员会选择了保守策略：**如果你写了析构但没显式写移动，编译器不敢猜你的移动是否安全**，干脆不生成。于是移动请求退化成拷贝。

顺带一提：声明了析构后**拷贝操作仍会生成**，但标准已把这一生成标记为 **deprecated**（因为"需要自定义析构"几乎总是意味着"需要自定义拷贝"）。实测要注意：clang 23.1.0 把这条诊断拆成了独立开关，`-Wall` / `-Wextra` 都**不会**触发它，必须显式加 `-Wdeprecated-copy-with-user-provided-dtor`。实测命令与告警原文：

```
$ clang++ -std=c++17 -Wall -Wdeprecated-copy-with-user-provided-dtor probe.cpp
probe.cpp:11:30: warning: definition of implicit copy constructor for 'W_Dtor'
        is deprecated because it has a user-provided destructor
        [-Wdeprecated-copy-with-user-provided-dtor]
probe.cpp:15:26: note: in implicit copy constructor for 'W_Dtor' first required here
```

---

## 完整可运行例子

把上面的几组类型一次性打印出生成规则矩阵（clang 23.1.0，`-std=c++11` 起各标准结果一致）：

```cpp
#include <iostream>
#include <type_traits>

struct Empty {};
struct DtorOnly { ~DtorOnly() {} };
struct CopyCtorOnly { CopyCtorOnly() = default; CopyCtorOnly(const CopyCtorOnly&) {} };
struct MoveCtorOnly { MoveCtorOnly(MoveCtorOnly&&) {} };

template <typename T>
void row(const char* name) {
    std::cout << name
              << "  copyC=" << std::is_copy_constructible<T>::value
              << "  moveC=" << std::is_move_constructible<T>::value
              << "  copyA=" << std::is_copy_assignable<T>::value
              << "  moveA=" << std::is_move_assignable<T>::value
              << "  triviallyCopyable=" << std::is_trivially_copyable<T>::value << "\n";
}

int main() {
    std::cout << std::boolalpha;
    row<Empty>       ("Empty       ");
    row<DtorOnly>    ("DtorOnly    ");
    row<CopyCtorOnly>("CopyCtorOnly");
    row<MoveCtorOnly>("MoveCtorOnly");
    return 0;
}
```

clang 23.1.0 实测输出：

```
Empty         copyC=true  moveC=true  copyA=true  moveA=true  triviallyCopyable=true
DtorOnly      copyC=true  moveC=true  copyA=true  moveA=true  triviallyCopyable=false
CopyCtorOnly  copyC=true  moveC=true  copyA=true  moveA=true  triviallyCopyable=false
MoveCtorOnly  copyC=false  moveC=true  copyA=false  moveA=false  triviallyCopyable=false
```

`MoveCtorOnly` 那一行最值得记住：**声明了移动操作，拷贝操作就被删除了**——想让 `vector` 既支持移动又支持拷贝，五个都得管。

---

## `=default` 写在类内还是 .cpp：决定类是否 trivial

`=default` 的位置会改变类的性质。实测（clang 23.1.0，`-std=c++17`）：

```cpp
struct Triv {};                                         // 什么都不写
struct InlineDefault { ~InlineDefault() = default; };   // 类内 = default
struct OutOfLine { ~OutOfLine(); };                     // 类内只声明
OutOfLine::~OutOfLine() = default;                      // 类外（.cpp）= default
struct UserProvided { ~UserProvided(); };
UserProvided::~UserProvided() {}                        // 类外手写空函数体
```

实测输出：

```
Triv         : trivially_destructible = true,  trivially_copyable = true
InlineDefault: trivially_destructible = true,  trivially_copyable = true
OutOfLine    : trivially_destructible = false, trivially_copyable = false
UserProvided : trivially_destructible = false, trivially_copyable = false
```

**规律：只有"在第一次声明处就 `=default`"才算 trivial。** 类内 `~X() = default;` → trivial；先声明 `~X();`、再到 .cpp 写 `X::~X() = default;` → **非 trivial**（这在标准里叫 user-provided）。这纠正一个常见误解——"把 `=default` 挪到 .cpp 能保持 trivial"是错的，恰恰相反。

那为什么还要把 `=default` 放到 .cpp？真正的理由有三个：① pimpl 惯用法里析构必须在看到完整类型之后才能定义（见 Item 22）；② 减少头文件暴露的内联代码，控制二进制体积；③ ABI 稳定。

---

## 常见错误（新手踩坑）

1. **写了析构忘了补移动操作**。直接后果：容器扩容退回拷贝（Item 14），性能静默退化；更糟的是编译器生成的浅拷贝可能 double free。
2. **以为 `is_move_constructible == true` 就说明有移动构造**。错——只要可拷贝，这个 trait 就是 true。要看真实行为得用计数器探针。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 拷贝 | `memcpy`（总是可用） | 拷贝构造/赋值（可能被抑制） | C++ 有资源管理 |
| 析构 | 手动 `free` | 自动调用析构函数 | RAII |
| 生成规则 | 不适用 | 写一个影响其余四个 | C++ 的保守策略 |

**一句话总结：** C 程序员记住——C++ 的类有 5 个（含默认构造是 6 个）特殊成员函数，**声明任何一个都会影响其余的自动生成**。最常踩的坑：写了析构 → 移动被抑制 → `vector` 扩容退回拷贝 → 性能差。优先级：先试 Rule of Zero（把资源交给标准库成员），必须自己管资源时再上 Rule of Five，移动操作记得标 `noexcept`。

---

## HFT 关联

- **移动被抑制导致扩容拷贝**：`Order` 写了析构但没写移动构造 → 扩容退回拷贝 → 延迟尖峰。这是 HFT C++ 性能的隐形坑，而且 `is_move_constructible` 不会告诉你。
- **RAII 资源管理**：HFT 代码大量用 RAII 管理锁、文件描述符、共享内存——写析构函数是常态，必须同时补移动操作。
- **结构体保持 trivial**：协议结构体不写任何特殊成员函数（Rule of Zero），保持 trivially copyable，可直接 `memcpy` 进出网络缓冲区，并用 `static_assert(std::is_trivially_copyable_v<Header>)` 在编译期锁死这个性质。

---

## 自测题

**1. 声明了移动构造后，拷贝构造还会自动生成吗？声明了析构呢（C++11）？**
<details><summary>答案</summary>
声明移动构造后，拷贝构造**不再自动生成**（被定义为 deleted）。声明析构函数（C++11 起）：移动构造与移动赋值**不会**生成；拷贝构造与拷贝赋值**仍会**生成，但这一生成已被标准标记为 **deprecated**——"需要自定义析构"几乎总是意味着"需要自定义拷贝"，所以不要依赖它。
</details>

**2. "大五律"是哪五个特殊成员函数？**
<details><summary>答案</summary>
析构函数、拷贝构造函数、拷贝赋值运算符、移动构造函数、移动赋值运算符。含义：如果需要自定义其中任何一个（通常因为类持有资源），就应该把五个都显式声明清楚（`=default` / `=delete` / 手写），否则编译器按抑制规则生成的行为很可能不符合意图。
</details>

**3. 为什么写了析构函数后移动构造会被抑制？有什么性能影响？**
<details><summary>答案</summary>
标准认为"用户写了析构函数"意味着该类在手动管理资源，此时逐成员移动很可能是错的，所以干脆不生成——移动请求退化成拷贝。性能影响：容器扩容、`push_back`、排序等搬移场合，本该 O(1) 的指针搬移退化成深拷贝，表现为扩容时的延迟尖峰，而且是**静默**的。
</details>

**4. 为什么说 `is_move_constructible` 会骗人？怎么才能看到真实行为？**
<details><summary>答案</summary>
它问的是"能否用 `T&&` 构造出 `T`"——而 `T&&` 能绑到 `const T&` 上去调拷贝构造。所以只要类可拷贝它就是 true，与是否存在移动构造无关。要看真实行为，用计数器探针：让类持有一个成员，其拷贝/移动构造分别计数，然后发起移动请求看计数器（实测 `W_Dtor` 的 `copies=1, moves=0`）。
</details>

**5. "写了任何一个就五个都声明"为什么重要？**
<details><summary>答案</summary>
因为生成规则（谁抑制谁）繁琐且随标准演进，靠记忆容易出错，漏掉某个就可能出现"编译通过但行为错误/性能退化"。显式声明五个把意图写进代码：既避免编译器生成错误的浅拷贝/缺失的移动，也让后来者一眼看清这个类的拷贝/移动语义。
</details>

**6. `=default` 写在类内和写在 .cpp 里，对类的 trivial 性有什么影响？**
<details><summary>答案</summary>
写在**类内第一次声明处** → 仍然 trivial（非 user-provided）；先声明、再到 .cpp 写 `=default` → **非 trivial**（user-provided）。实测：`InlineDefault` 的 `is_trivially_copyable` 为 `true`，`OutOfLine` 为 `false`。所以"挪到 .cpp 能保持 trivial"是错的——真正理由是 pimpl、二进制体积与 ABI 稳定。
</details>

**7. 下面代码有什么问题？**
```cpp
class Buffer {
    char* data;  size_t size;
public:
    Buffer(size_t n) : data(new char[n]), size(n) {}
    ~Buffer() { delete[] data; }
};
std::vector<Buffer> v;  v.push_back(Buffer(64));   // 悬垂指针 + double free
```
<details><summary>答案</summary>
违反大五律，有严重 bug。有裸指针和析构函数却没有拷贝/移动操作：移动构造被抑制，编译器生成的拷贝构造做**浅拷贝**（只复制 `data` 指针）。`v.push_back(Buffer(64))` 时临时对象被浅拷贝进 vector，随后临时对象析构执行 `delete[] data`，vector 里的元素就持有**悬垂指针**；后续再拷贝、扩容或析构时会重复释放同一块内存（double free）与读野指针。

修正：优先 Rule of Zero——把 `char* data` 换成 `std::vector<char>` 或 `std::unique_ptr<char[]>`，编译器生成的移动/拷贝天然正确（且移动自动 `noexcept`）。
</details>

---

## 参考与延伸

- 下一章：[第 4 章 智能指针](../ch04-smart-pointers/README.md)
- 回到：[第 3 章 移步现代 C++](README.md)
