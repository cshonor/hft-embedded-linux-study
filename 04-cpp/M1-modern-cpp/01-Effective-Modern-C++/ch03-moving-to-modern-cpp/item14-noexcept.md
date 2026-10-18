# Item 14：声明 noexcept 如果函数保证不抛

> 第 3 章 移步现代 C++ · Item 14 · 上一节：[Item 13 const_iterator](item13-const-iterator.md)

## 为什么要学这个（先建立直觉）

C 没有异常机制，C 程序员习惯用返回值表示错误：`if (data == NULL) return -1;`。C++ 有异常，函数可以 `throw` 而不返回错误码。但异常不是免费的——编译器要生成 unwind 表，运行时栈展开有开销。C++11 的 `noexcept` 就是给编译器的承诺："这个函数保证不抛"，编译器据此省掉异常处理代码、做更强的优化。

**但 `noexcept` 真正值钱的地方不在优化，而在它是一份接口契约，会被标准库读取。** `std::vector` 在 `push_back` 扩容、搬移旧元素时，会检查元素的移动构造是不是 `noexcept`。是 → 用移动（每个元素 O(1)）；不是 → 退回拷贝（安全但慢）。这个检查是**编译期**的（`std::is_nothrow_move_constructible` + `std::move_if_noexcept`）。

也就是说：你写了移动构造但忘了标 `noexcept`，容器根本不敢用，悄悄退化成拷贝。**代码能跑、单测能过、只是慢**——而且慢在一个最不该慢的地方：扩容那一瞬间。

---

## 这节讲什么

1. `vector` 扩容时 `move_if_noexcept` 的实测：移动构造标与不标 `noexcept`，构造次数差多少；
2. `noexcept` 是承诺不是建议：标错并抛异常的后果（`terminate`）与编译告警；
3. `swap` 与析构的隐式 `noexcept`；
4. 条件 `noexcept` 与 `noexcept` 运算符。

---

## 实测一：vector 扩容走 move 还是 copy

用一个带计数器的类模板，两种实例化**只差移动构造上的 `noexcept`**（`noexcept(NX)` 叫**条件 noexcept**），其余代码完全相同：

```cpp
#include <iostream>
#include <vector>
#include <type_traits>
#include <utility>

template <bool NX>
struct Counted {
    static int copies, moves;
    int id = 0;
    Counted() = default;
    explicit Counted(int i) : id(i) {}
    Counted(const Counted& o) : id(o.id) { ++copies; }
    Counted(Counted&& o) noexcept(NX) : id(o.id) { o.id = -1; ++moves; }
};
template <bool NX> int Counted<NX>::copies = 0;
template <bool NX> int Counted<NX>::moves  = 0;

template <bool NX>
void run(const char* tag) {
    std::vector<Counted<NX>> v;
    for (int i = 0; i < 9; ++i) {
        Counted<NX> c(i);
        v.push_back(std::move(c));      // 容量不足时触发搬移
    }
    std::cout << tag << "  is_nothrow_move_constructible="
              << std::is_nothrow_move_constructible<Counted<NX>>::value
              << "  copies=" << Counted<NX>::copies << "  moves=" << Counted<NX>::moves << "\n";
}

int main() {
    std::cout << std::boolalpha;
    run<true >("move ctor noexcept   ");
    run<false>("move ctor NOT noexcp ");
    return 0;
}
```

连续 `push_back(std::move(c))` 九次，容量按 0→1→2→4→8→16 翻倍。clang 23.1.0（`-std=c++17 -Wall`）实测输出：

```
move ctor noexcept     is_nothrow_move_constructible=true  copies=0  moves=24
move ctor NOT noexcp   is_nothrow_move_constructible=false  copies=15  moves=9
```

| 版本 | `push_back` 直接构造 | 扩容搬移旧元素 | 合计 |
|------|------------------|-----------|-----|
| 标了 `noexcept` | 9 次移动 | 1+2+4+8 = **15 次移动** | `moves=24, copies=0` |
| 没标 `noexcept` | 9 次移动 | 15 次**拷贝** | `moves=9, copies=15` |

**唯一的差别就是那 15 次搬移走的是移动还是拷贝。** 当元素里带着 `std::string`、堆缓冲区或锁句柄时，这 15 次就是 15 次深拷贝——正是 HFT 里"扩容延迟尖峰"的来源。

---

## 实测二：noexcept 是承诺，不是建议

标了 `noexcept` 的函数若真让异常逃逸，标准规定调用 `std::terminate()`——程序直接终止，没有栈展开，也没有 catch 的机会：

```cpp
#include <iostream>
#include <stdexcept>

void boom() noexcept {
    throw std::runtime_error("oops");   // 从 noexcept 函数逃逸
}

int main() {
    std::cout << "before boom()\n";
    boom();
    std::cout << "after boom()\n";      // 永远不会打印
    return 0;
}
```

实测（clang 23.1.0）：编译期先给一条告警，运行中 abort，退出码 134（SIGABRT）。

```
warning: 'boom' has a non-throwing exception specification but can still throw [-Wexceptions]
note: function declared non-throwing here

before boom()
libc++abi: terminating due to uncaught exception of type std::runtime_error: oops
```

如果异常不是直接 `throw`，而是来自内部的 `std::make_shared`、`vector::push_back`（可能抛 `bad_alloc`），编译器未必告警——但 `terminate` 一样会发生。同理，析构函数自 C++11 起**默认就是 `noexcept`**，在析构里抛异常同样 `terminate`。

---

## 实测三：swap / 析构的默认 noexcept

有一批操作编译器会**自动**标 `noexcept`（除非成员/基类显式禁止），不需要你动手：

| 函数 | 默认是否 `noexcept` | 说明 |
|------|-------------------|------|
| 析构函数 | **是**（C++11 起） | 只有显式 `noexcept(false)` 才能改变 |
| 隐式生成的拷贝/移动构造与赋值 | 由成员决定 | 所有成员的对应操作都 `noexcept` 才是 |
| 隐式生成的默认构造 | 由成员决定 | 同上 |
| `operator delete` / `delete[]` | 是 | 释放内存不该失败 |

自己写 `swap` 时应该用条件 `noexcept`，让"是否不抛"跟着元素类型走：

```cpp
template <typename T>
void my_swap(T& a, T& b) noexcept(std::is_nothrow_move_constructible<T>::value &&
                                  std::is_nothrow_move_assignable<T>::value) {
    T tmp = std::move(a);
    a = std::move(b);
    b = std::move(tmp);
}
```

`noexcept(表达式)` 里的操作数不会被求值，只做"这个表达式会不会抛"的编译期判断——这就是 **`noexcept` 运算符**。

---

## 实测四：C++17 起 noexcept 进入函数类型

C++17 之前 `noexcept` 只是函数属性，不属于类型；C++17（P0012R1）把它**并入函数类型**。于是函数指针的赋值规则变了：

```cpp
void safe() noexcept { }
void risky() { }

void (*p1)() noexcept = safe;   // OK：noexcept → noexcept
void (*p2)()          = risky;   // OK：非 noexcept → 非 noexcept
void (*p3)() noexcept = risky;   // C++17 起：非法！非 noexcept → noexcept
```

跨标准实测报错原文（clang 23.1.0）：

```
-std=c++11 : error: target exception specification is not superset of source
-std=c++14 : error: target exception specification is not superset of source
-std=c++17 : error: cannot initialize a variable of type 'void (*)() noexcept'
                    with an lvalue of type 'void ()': different exception specifications
-std=c++20 : 同 c++17
```

C++11/14 也报错，但**理由不同**：前者是函数自身的异常规范太窄，后者是**类型不匹配**。类型层面的差异可以用 `std::is_same` 直接看到：

```cpp
std::cout << std::is_same<void(), void() noexcept>::value                  // c++11/14: 1   c++17/20: 0
          << std::is_convertible<void (*)() noexcept, void (*)()>::value;  // 全部标准: 1
```

反向（把 `noexcept` 函数指针赋给普通函数指针）在所有标准下都合法——C++17 专门加了一条隐式转换，避免"加了 `noexcept` 反而破坏老代码"。实践含义：**给已有函数加 `noexcept` 通常是安全的，但撤掉 `noexcept` 可能破坏调用方**。所以它叫接口契约。

---

## 完整可运行例子

第二个程序：把"条件 noexcept + 析构默认 noexcept"两件静态性质一次打印出来（`-std=c++17 -Wall`）。

```cpp
#include <iostream>
#include <type_traits>
#include <utility>

struct Plain         { int a = 0; double b = 0.0; };
struct WithUserDtor  { ~WithUserDtor() {} };
struct WithThrowDtor { ~WithThrowDtor() noexcept(false) {} };

struct NoexceptMove { NoexceptMove(NoexceptMove&&) noexcept {} NoexceptMove& operator=(NoexceptMove&&) noexcept { return *this; } };
struct ThrowingMove { ThrowingMove(ThrowingMove&&) {} ThrowingMove& operator=(ThrowingMove&&) { return *this; } };

// 条件 noexcept：my_swap 是否 noexcept 取决于元素类型
template <typename T>
void my_swap(T& a, T& b) noexcept(std::is_nothrow_move_constructible<T>::value &&
                                  std::is_nothrow_move_assignable<T>::value) {
    T tmp = std::move(a);
    a = std::move(b);
    b = std::move(tmp);
}

int main() {
    std::cout << std::boolalpha;
    std::cout << "Plain         nothrow dtor = " << std::is_nothrow_destructible<Plain>::value << "\n"
              << "WithUserDtor  nothrow dtor = " << std::is_nothrow_destructible<WithUserDtor>::value << "\n"
              << "WithThrowDtor nothrow dtor = " << std::is_nothrow_destructible<WithThrowDtor>::value << "\n";
    std::cout << "my_swap<NoexceptMove> noexcept = " << noexcept(my_swap(std::declval<NoexceptMove&>(), std::declval<NoexceptMove&>())) << "\n";
    std::cout << "my_swap<ThrowingMove> noexcept = " << noexcept(my_swap(std::declval<ThrowingMove&>(), std::declval<ThrowingMove&>())) << "\n";
    return 0;
}
```

clang 23.1.0 实测输出：

```
Plain         nothrow dtor = true
WithUserDtor  nothrow dtor = true
WithThrowDtor nothrow dtor = false
my_swap<NoexceptMove> noexcept = true
my_swap<ThrowingMove> noexcept = false
```

注意 `WithUserDtor` 那行：**哪怕是手写空函数体的析构，也仍然是 `noexcept`**。而 `my_swap<ThrowingMove>` 为 `false`，说明条件 `noexcept` 把"元素会不会抛"如实传导到了 `swap` 上——这正是 Item 14 想让你养成的写法。

---

## 常见错误（新手踩坑）

1. **移动构造 / 移动赋值忘了标 `noexcept`**。本 Item 的头号实践项。资源搬移只是拷指针、置空，本不会抛；但编译器按"可能抛"处理，容器扩容退回拷贝（实测差 15 次拷贝）。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 错误处理 | 返回值（`return -1`） | 异常（`throw`）或返回值 | C++ 有异常机制 |
| 不抛保证 | 不适用 | `noexcept` | 编译器优化 + 接口契约 |
| 析构抛异常 | 不适用 | 禁止（`terminate`） | 异常安全保证 |

**一句话总结：** `noexcept` 是"我保证不抛"的**契约**，不是给编译器的提示。析构和 `swap` 默认就是 `noexcept`，你真正要动手标的是**移动构造**和**移动赋值**。

---

## HFT 关联

- **扩容延迟尖峰**：`vector<Order>` 在 `push_back` 扩容时，`Order` 的移动构造必须 `noexcept` 才走移动，否则退回拷贝，订单簿重建时出现延迟尖峰。这是 HFT C++ 性能的**隐形开关**——实测同样 9 次 `push_back`，差 15 次拷贝。
- **`move_if_noexcept` 的作用**：容器搬移元素时要维持强异常保证——搬移途中抛异常不能破坏原容器。移动不抛才敢用移动，否则用拷贝。所以"移动更快"不是你决定的，是 `noexcept` 决定的。
- **代码体积**：`noexcept` 让编译器省略 unwind 表，减小二进制体积，间接降低 i-cache 压力（次要收益，主要收益仍是容器分派）。

---

## 自测题

**1. `vector` 扩容时如何决定搬移元素用移动还是拷贝？**
<details><summary>答案</summary>
编译期判断元素类型的移动构造是否 `noexcept`（`std::is_nothrow_move_constructible`）：是 → 用移动构造搬移，快且能提供强异常保证；否 → 退回拷贝构造，因为容器必须保证"搬移途中抛异常也不破坏原容器"。实现上走的是 `std::move_if_noexcept`。
</details>

**2. 实测里两种 `Counted` 的 `copies`/`moves` 分别是多少？为什么？**
<details><summary>答案</summary>
标 `noexcept`：`copies=0, moves=24`；没标：`copies=15, moves=9`。9 次是 `push_back(std::move(c))` 直接构造新元素（不受 `noexcept` 影响）；容量按 1→2→4→8→16 翻倍共搬移 15 次旧元素——这 15 次才是"移动 vs 拷贝"的分水岭。
</details>

**3. 标了 `noexcept` 却抛了异常会怎样？编译器会拦吗？**
<details><summary>答案</summary>
不会拦，只给 `-Wexceptions` 告警；运行时调用 `std::terminate()`，进程 abort（实测退出码 134）。若异常来自内部的 `make_shared`/`push_back`（可能抛 `bad_alloc`），连告警都可能没有。
</details>

**4. 为什么漏标 `noexcept` 比错标安全？**
<details><summary>答案</summary>
漏标的后果只是"性能没那么好"——容器退回拷贝，代码仍正确。错标（标了却可能抛）的后果是 `terminate`：进程崩溃、订单状态不明。所以只对确实不会抛、且经过审查的函数标注。
</details>

**5. C++17 起 `noexcept` 进入函数类型后，函数指针赋值规则怎么变的？**
<details><summary>答案</summary>
普通函数指针 → `noexcept` 函数指针变非法（C++17/20 报 `different exception specifications`；C++11/14 报 `target exception specification is not superset of source`）。反向 `noexcept` → 普通仍然合法（标准特意加了这条隐式转换）。`std::is_same<void(), void() noexcept>` 从 1 变成 0。
</details>

**6. 下面代码有什么问题？**
```cpp
class Widget {
public:
    Widget(Widget&& o) { data_ = o.data_; o.data_ = nullptr; }
private:
    int* data_;
};
std::vector<Widget> v;
v.push_back(std::move(w));
```
<details><summary>答案</summary>
移动构造没标 `noexcept`。它只是搬指针、置空，本身不会抛；但编译器按"可能抛"处理，`vector<Widget>` 扩容（以及 `resize`、`insert` 等需要搬移的场合）会保守地改用拷贝构造，`push_back(std::move(w))` 的移动优势被浪费。修正：`Widget(Widget&& o) noexcept : data_(o.data_) { o.data_ = nullptr; }`。
</details>

---

## 参考与延伸

- 下一节：[Item 15 constexpr](item15-constexpr.md)
- 回到：[第 3 章 移步现代 C++](README.md)
