// 16.2 非类型模板参数：std::array<T,N> 的 N 为什么必须写进 template<>
//
// 编译：g++ -std=c++20 -Wall -Wextra 16.2-array-nttp.cpp -o demo && ./demo
//
// 三种写法对照：
//   写法 1  template<class T, size_t N> f(const std::array<T,N>&)   -> 任意长度
//   写法 2  f(const std::array<int,6>&)                             -> 只收 N=6
//   错误写法 f(const std::array<int, size_t N>&)                    -> 编译失败
//            （参数列表里的 size_t N 是「声明符」，模板实参位只接受「表达式」）

#include <array>
#include <cstdio>
#include <span>
#include <type_traits>

// ---- 写法 1：T 与 N 都是模板参数，N 由编译器推导 ----
template <typename T, std::size_t N>
void show_array(const std::array<T, N>& arr)
{
    std::printf("N=%zu size=%zu  ", N, arr.size());
    for (auto v : arr) std::printf("%d ", static_cast<int>(v));
    std::printf("\n");

    // N 是常量表达式，可直接用于编译期语境
    static_assert(N == std::tuple_size_v<std::array<T, N>>, "N 与 tuple_size 恒等");
    // 注意：arr.size() 虽然是 constexpr 成员函数，但 arr 是形参引用，
    // 不是常量表达式 —— 编译期要长度只能用 N（见下方注释里的反例）
    std::array<T, N> copy = arr;      // 体内还能用 N 再声明一个同型数组
    (void)copy;
}

// ---- 写法 2：固定长度，只收 N=6 ----
void show_fixed(const std::array<int, 6>& arr)
{
    std::printf("fixed size=%zu\n", arr.size());
}

// ---- 模板版求和：每个不同的 N 各实例化一份 ----
template <typename T, std::size_t N>
T sum_tmpl(const std::array<T, N>& a)
{
    T s = T{};
    for (std::size_t i = 0; i < N; ++i) s += a[i];
    return s;
}

// ---- C++20 span 版：只实例化一份，代价是长度降级为运行期值 ----
long sum_span(const std::span<const int> sp)
{
    long t = 0;
    for (std::size_t i = 0; i < sp.size(); ++i) t += sp[i];
    return t;
}

int main()
{
    std::array<int, 3>   a1{1, 2, 3};
    std::array<int, 5>   a2{10, 20, 30, 40, 50};
    std::array<float, 4> a3{1.5f, 2.5f, 3.5f, 4.5f};
    std::array<int, 8>   a4{1, 2, 3, 4, 5, 6, 7, 8};

    show_array(a1);          // N=3
    show_array(a2);          // N=5
    show_array(a3);          // T=float, N=4
    show_array(a4);          // N=8

    std::array<int, 6> a6{1, 2, 3, 4, 5, 6};
    show_fixed(a6);

    // 类型层面：不同 N / 不同 T 都是不同类型
    std::printf("same type (3 vs 5)?      %d\n",
                static_cast<int>(std::is_same_v<std::array<int, 3>, std::array<int, 5>>));
    std::printf("same type (int,3 vs long,3)? %d\n",
                static_cast<int>(std::is_same_v<std::array<int, 3>, std::array<long, 3>>));

    // N = 0 是合法长度
    std::array<int, 0> a0{};
    std::printf("N=0: size=%zu sizeof=%zu data=%p begin==end? %d\n",
                a0.size(), sizeof(a0), static_cast<void*>(a0.data()),
                static_cast<int>(a0.begin() == a0.end()));
    show_array(a0);

    // C++17 CTAD：调用方连 <int,3> 都不用写，但 N 依旧是类型的一部分
    std::array ctad{1, 2, 3};
    std::printf("CTAD: is array<int,3>? %d  N=%zu\n",
                static_cast<int>(std::is_same_v<decltype(ctad), std::array<int, 3>>),
                ctad.size());

    std::printf("sum_tmpl(a1)=%d sum_tmpl(a4)=%d sum_span(a2)=%ld\n",
                sum_tmpl(a1), sum_tmpl(a4),
                static_cast<long>(sum_span(std::span<const int>(a2.data(), a2.size()))));

    // 按值传参的拷贝代价：不退化 == 真的整块拷贝
    std::array<int, 256> big{};
    std::printf("sizeof(array<int,256>)=%zu\n", sizeof(big));

    return 0;
}

// ---------------------------------------------------------------------------
// 反例（取消注释即编译失败）：
//
//   void bad(const std::array<int, std::size_t N>& a);
//   gcc 13.2     : error: template argument 2 is invalid
//                  error: invalid template-id
//   clang 18.1.0 : error: type-id cannot have a name
//                  error: template argument for non-type template parameter
//                         must be an expression
//
// 反例 2：形参引用上的 .size() 不是常量表达式
//
//   template<class T, size_t N> void g(const std::array<T,N>& arr) {
//       static_assert(N == arr.size(), "...");   // ❌
//   }
//   gcc 13.2     : error: non-constant condition for static assertion
//                  error: 'arr' is not a constant expression
//   clang 18.1.0 : error: static assertion expression is not an integral
//                         constant expression
//
// 反例 3：array<T,N> 不能隐式转成 array<const T,N>
//
//   void h(const std::array<const int, 3>&); h(a1);
//   error: invalid initialization of reference of type
//          'const std::array<const int, 3>&' from expression of type
//          'std::array<int, 3>'
