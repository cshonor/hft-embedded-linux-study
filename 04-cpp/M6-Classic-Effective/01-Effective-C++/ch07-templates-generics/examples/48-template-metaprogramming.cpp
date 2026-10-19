// 48-template-metaprogramming.cpp
// 编译：g++ -std=c++20 -O1 -S -masm=intel 48-template-metaprogramming.cpp
// 实测环境：Compiler Explorer / g132（gcc 13.2.0, x86-64 linux）
//
// 目的：条款 48「了解模板元编程」的实证 ——
//   1) constexpr / static_assert 把计算挪到编译期，产物里不留代码
//   2) traits 偏特化把运行期分支变成编译期选型
//   3) 每个实例化都是独立符号 -> 代码膨胀
//
// ★ 最强证据（gcc 13.2 -O1 实测）：constexpr 版本在汇编里彻底消失，
//   而运行期版本生成完整递归调用。
//
//   预期汇编（-O1）：
//     fact_rt(int):        push rbx / mov ebx,edi / lea edi,[rdi-1]
//                          call fact_rt / imul eax,ebx / pop rbx / ret
//     check_rt():          mov edi,5 / call fact_rt / sal eax,2 / ret
//   注意：**没有 fact 函数** —— constexpr 版被 static_assert 全部消化，
//   编译器判定结果后直接扔掉。
//
//   -O0 下代码膨胀证据：
//     use_a(): call int process<int>(int)
//     use_b(): call int process<double>(double)
//     use_c(): call int process<char>(char)
//     同一份模板体，三个类型 = 三个独立符号。

#include <cstddef>
#include <type_traits>

// ---------- 1. 编译期计算 ----------

constexpr std::size_t fact(std::size_t n) {
    return n <= 1 ? 1 : n * fact(n - 1);
}

// 编译期算出 10! —— 若 fact 不是 constexpr，这两行会编译失败
static_assert(fact(10) == 3628800, "10! = 3628800");
static_assert(fact(20) > 0);

// constexpr 的真正价值：定长结构体的大小也能编译期确定
struct Packet {
    std::byte raw[fact(5)];      // 120 字节，编译期定死
    std::byte hdr[fact(2)];      //   2 字节
};
static_assert(sizeof(Packet) == 122, "122 字节，无 padding");

// 对照：运行期版本必须生成递归代码，且数组大小运行期才知道
static int fact_rt(int n) { return n <= 1 ? 1 : n * fact_rt(n - 1); }
int check_rt() {
    int buf[fact_rt(5)];         // 运行期才知道大小
    return static_cast<int>(sizeof(buf));
}

// ---------- 2. traits：偏特化实现「编译期 if-else」 ----------

// 不用运行期分支，靠偏特化在编译期选路
template <bool B>
struct impl { static const char* name() { return "false"; } };

template <>
struct impl<true> { static const char* name() { return "true"; } };

static_assert(sizeof(impl<true>) == 1, "空类，无数据成员");

// 标准库的 type_traits 就是这个套路
static_assert(std::is_same_v<std::remove_reference_t<int&>, int>);
static_assert(std::is_same_v<std::remove_cv_t<const int>, int>);
static_assert(std::is_pointer_v<int*>);

// 非类型模板参数：把「值」提升成「类型的一部分」
template <std::size_t N>
struct FixedBuf {
    std::byte data[N];
};
static_assert(sizeof(FixedBuf<128>) == 128);

// traits 做编译期分派：运行期 if → 编译期选型
template <class T>
constexpr bool is_trivially_copyable_v = std::is_trivially_copyable_v<T>;
static_assert(is_trivially_copyable_v<int>);

// ---------- 3. 代码膨胀：每个实例化都是独立符号 ----------

template <class T>
int process(T v) { return v * 2; }

int use_a() { return process(1); }
int use_b() { return process(2.0); }      // process<double>
int use_c() { return process('x'); }      // process<char>

// 非依赖调用只实例化一次：process(3) 与 T 无关
template <class T>
int compute(T v) { return process(3) + v; }
int calc() { return compute(7) + compute(1.5); }