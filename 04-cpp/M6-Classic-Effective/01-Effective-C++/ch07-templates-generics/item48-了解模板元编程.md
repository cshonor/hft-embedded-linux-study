# 条款 48：了解模板元编程（Be aware of template metaprogramming）

## 本节讲什么

模板元编程（TMP）= **用 C++ 的类型系统写程序**。既然模板参数可以是类型、模板实参可以是编译期常量，那么类型和常量就能参与计算——把原本运行期做的事挪到编译期。

Meyers 的判断很直接：**TMP 能做，但"能"不等于"该"**。本节讲清三件事——它能干什么、代价是什么、什么时候该用什么时候该躲。

> 实测环境：Compiler Explorer，`g132`（gcc 13.2.0，x86-64 Linux，`-O1 -S -masm=intel` / `-O0 -S`）。
> 源码见 `examples/48-template-metaprogramming.cpp`，全部 `static_assert` 与汇编均为实测。

---

## 一、最强证据：constexpr 版在汇编里彻底消失

同一份源码里放两版阶乘——`constexpr` 版和普通版。对比 `-O1` 汇编：

```cpp
constexpr std::size_t fact(std::size_t n) { return n <= 1 ? 1 : n * fact(n - 1); }
static_assert(fact(10) == 3628800, "10! = 3628800");

static int fact_rt(int n) { return n <= 1 ? 1 : n * fact_rt(n - 1); }
int check_rt() { int buf[fact_rt(5)]; return (int)sizeof(buf); }
```

**实测汇编（gcc 13.2 `-O1`）**：

```asm
fact_rt(int):
        mov     eax, 1
        cmp     edi, 1
        jg      .L8
        ret
.L8:
        push    rbx
        mov     ebx, edi
        lea     edi, [rdi-1]
        call    fact_rt(int)        ; ← 真递归
        imul    eax, ebx
        pop     rbx
        ret
check_rt():
        sub     rsp, 8
        mov     edi, 5
        call    fact_rt(int)        ; ← 运行期算
        sal     eax, 2
        add     rsp, 8
        ret
```

**注意汇编里根本没有 `fact` 这个函数。** 编译器在编译期把 `fact(10)` 算成 3628800，喂给 `static_assert` 检查通过后，就把这段计算**整个扔掉了**。而运行期版本生成了完整的递归调用序列（push/call/imul）。

这就是 TMP 的本质：**没有代码，就没有运行期开销，也没有分支预测失败、没有 cache miss**。

### 1.1 定长结构体的编译期定尺

```cpp
constexpr std::size_t fact(std::size_t n) { return n <= 1 ? 1 : n * fact(n - 1); }

struct Packet {
    std::byte raw[fact(5)];      // 120 字节
    std::byte hdr[fact(2)];      //   2 字节
};
static_assert(sizeof(Packet) == 122);   // ✅ 通过
```

HFT 相关：定长报文结构体的字段长度可以直接写成编译期表达式，`static_assert` 立刻验证布局符合协议。这是 **encode/decode 零开销的前提**——`sizeof(Packet) == 122` 一旦不成立，编译就失败，而不是上线后长度错位。

（→ 与 [3.5.6](../../../M0-entry-syntax/01-C++Primer/ch03-strings-vectors-arrays/3.5-arrays/3.5.6-C++在内建数组之上补的三层.md) 的 `std::array<char,16>` 思路一致：协议字段用编译期常量钉死。）

---

## 二、traits：把运行期分支变成编译期选型

TMP 最常见的落地形态是 **traits 类**——用模板偏特化实现「编译期的 if-else」。

```cpp
template <bool B>
struct impl { static const char* name() { return "false"; } };

template <>
struct impl<true> { static const char* name() { return "true"; } };

static_assert(sizeof(impl<true>) == 1);   // 空类，无数据成员
```

运行的是 `if (b)` 还是 `impl<b>::name()`，本质区别：**前者的分支在运行期**，编译器看不到；后者 `B` 是模板参数，**编译期就能定死走哪个特化**。

标准库的 `type_traits` 就是这个套路（全部实测通过）：

```cpp
static_assert(std::is_same_v<std::remove_reference_t<int&>, int>);
static_assert(std::is_same_v<std::remove_cv_t<const int>, int>);
static_assert(std::is_pointer_v<int*>);
```

**这正是条款 47（traits classes）的实现基础**——但 47 讲的是「怎么用 traits」，48 讲的是「模板体系整体能做到什么」，两者是上下游。

### 2.1 非类型模板参数：把值提升成类型的一部分

```cpp
template <std::size_t N>
struct FixedBuf { std::byte data[N]; };

static_assert(sizeof(FixedBuf<128>) == 128);   // ✅
```

`N` 是**非类型模板参数**，它进入类型系统后，编译器就知道 `FixedBuf<128>` 是和 `FixedBuf<64>` **完全不同的类型**。这条能力是后面条款 44（消除代码膨胀）的解法来源——把非类型参数换成函数参数或数据成员，就能让多个实例化共用一份实现。

---

## 三、代价：代码膨胀

TMP 不是免费的。每个实例化都是**独立的符号**——同一份模板体，`int`/`double`/`char` 各生成一份。

**实测 `-O0` 汇编**（`-O0` 是为了让实例化符号显形）：

```asm
use_a():
        mov     edi, 1
        call    int process<int>(int)        ; ← 独立符号
        ret
use_b():
        mov     rax, QWORD PTR .LC0[rip]
        movq    xmm0, rax
        call    int process<double>(double)   ; ← 又是独立符号
        ret
use_c():
        mov     edi, 120
        call    int process<char>(char)      ; ← 第三个
        ret
```

`process` 的函数体只有一行 `v * 2`，却因为三个实例化变成三份代码。模板参数越多实例化越多——**这段膨胀的源头是模板体系本身，与优化等级无关**（`-O1` 只是把函数体合并，不会让 `process<int>` 和 `process<double>` 变成同一个符号）。

### 3.1 非依赖调用只实例化一次

```cpp
template <class T> int process(T v) { return v * 2; }

template <class T>
int compute(T v) { return process(3) + v; }   // process(3) 与 T 无关
```

```asm
calc():
        mov     edi, 7
        call    int compute<int>(int)
        mov     ebx, eax
        mov     rax, QWORD PTR .LC1[rip]
        movq    xmm0, rax
        call    int compute<double>(double)
        add     eax, ebx
        ret
```

`compute<int>` 和 `compute<double>` 是两份（因为 `v` 依赖 T），但内部的 `process(3)` 是**非依赖调用**——编译器只生成一份 `process<int>`。这是消除膨胀的第一条原则（→ 条款 44）。

---

## 四、什么时候用、什么时候躲

| 场景 | 建议 | 理由 |
|------|------|------|
| 定长协议字段大小、offsetof 校验 | **用** | 编译期 `static_assert` 把错误前移，零运行期开销 |
| 编译期查表 / 常量表生成 | **用** | 产物是纯数据，不含代码 |
| 类型分派（同一算法多种类型） | **谨慎用** | 用 traits 替代运行期虚函数可省 vptr，但要看实例化规模 |
| 深度 TMP（元编程库） | **躲** | 编译期爆炸、报错信息无法读、工具链兼容性风险 |
| 表达式模板 / `std::span` 这类库 | **用（但只当用户）** | 实现难读，但用起来是零成本抽象 |

**判断标准**：TMP 的收益是「消除运行期开销」，代价是「编译期开销 + 二进制膨胀 + 可读性」。热路径上消除分支和虚调用值得；非热路径上通常不值得——除非编译期校验能替你挡住一类线上 bug。

**Meyers 的补充提醒**（原书条款 48 的落点）：TMP 让「C++ 的类型系统」和「C++ 的值语义」边界模糊起来。能力越大，越要清楚自己依赖的是语言保证还是某个库的实现细节——后者在库升级时会静默失效。

---

## 自测题

**1. 判断题**　`constexpr` 函数一定会出现在编译产物的符号表里（可以取地址传给函数指针）。

<details><summary>答案</summary>

**错**。本节实测就是反例：`fact` 是 `constexpr`，但它在汇编里**彻底消失**了。

原因：调用点全部发生在常量表达式上下文（`static_assert`、定长数组维度），编译器算完就把结果折进去、原函数不生成代码。只有在**必须生成函数体**的场景（取地址、`std::min` 之类按引用取参导致无法常量求值）才会真正落地。

```cpp
constexpr std::size_t fact(std::size_t n) { ... }
auto p = &fact;          // 强制生成符号，此时才有定义可取
```

</details>

**2. 应用题**　下面这段模板导致二进制膨胀，每个实例化生成一份几乎相同的代码：

```cpp
template <class T>
std::size_t payload_size(T t) {
    return header_len() + sizeof(T) + checksum_len();   // header_len() 是非成员函数
}
```

有 `Order` / `Trade` / `Quote` 三种类型调用它。给出**两条**消除膨胀的手段。

<details><summary>答案</summary>

**手段一：把非依赖部分抽出去**（→ 条款 44 的核心手法）

```cpp
// 非依赖调用：编译器只生成一份 payload_size_common
std::size_t payload_size_common(std::size_t obj_size);

template <class T>
std::size_t payload_size(T) {
    return payload_size_common(sizeof(T));   // 只有 sizeof(T) 依赖 T
}
```

实测印证：非依赖调用只实例化一次，`process(3)` 在 `compute<int>` 与 `compute<double>` 里共用一份 `process<int>`。

**手段二：把非类型模板参数换成函数参数或数据成员**（消除实例化本身）

```cpp
// 改前：sizeof(T) 是类型的一部分 -> 每种类型一个类
template <class T> struct Msg { T body[1]; };

// 改后：长度退化为构造参数 -> 一个类处理所有长度
struct Msg {
    std::byte body[1];
    explicit Msg(std::size_t n) { /* 按 n 初始化 */ }
};
```

这条正是条款 44 讲的「非类型模板参数导致的膨胀可以用函数参数或数据成员消除」。

</details>

**3. 踩坑题**　想把运行期的类型分支改成编译期分派，下面写法为什么没达到目的？

```cpp
template <class T>
void handle(const T& x) {
    if (std::is_pointer_v<T>) {         // ← 看起来是"编译期判断"
        handle_deref(*x);
    } else {
        handle_value(x);
    }
}
```

<details><summary>答案</summary>

`std::is_pointer_v<T>` **确实是编译期常量**，`if` 条件在每个实例化里都是常量。编译器通常会做**分支消除**（常量折叠 + 死代码删除），效果上接近编译期分派。

但这不是 TMP 的正确姿势，有两个问题：

1. **你把两个分支都写在了函数体里**，源码层面仍是一份含分支的代码。语义上依赖「编译器会优化掉」，而不是语言层面保证没有分支——`-O0` 下分支就在那里（实测 `-O0` 下 `use_b()` 里 `process(2.0)` 的浮点常量照样要 movq 加载）。

2. **两个分支都会实例化**，可调用文件体积仍按 T 增长。

TMP 的正路是**偏特化**：

```cpp
template <class T> struct handle_impl { static void go(const T& x) { handle_value(x); } };
template <class T> struct handle_impl<T*> { static void go(const T& x) { handle_deref(*x); } };

template <class T> void handle(const T& x) { handle_impl<T>::go(x); }
```

此时 `handle_impl<int>` 和 `handle_impl<int*>` 是两个类型，各自只有一条路径——**运行期分支从类型层面就不存在**，而不是靠优化器帮忙删掉。

</details>

---

← [条款 47：traits classes 表现类型信息](./item47-请使用traits-classes表现类型信息.md) | [条款 49：了解 new-handler 的行为](../ch08-new-delete/item49-了解new-handler的行为.md)