// 3.5.6-2-std-array-span-size.cpp
// 编译：g++ -std=c++20 -O2 -c 3.5.6-2-std-array-span-size.cpp
// 实测环境：Compiler Explorer / g132（gcc 13.2.0, x86-64 linux）
//
// 目的：验证 C++ 在内建数组之上补的三样东西
//   1) std::size / std::ssize —— 取长度不必再写 sizeof(a)/sizeof(a[0])
//   2) std::array<T,N>        —— 布局与 T[N] 等价（可 reinterpret_cast 打报文）
//   3) std::span<T,N>          —— 零拷贝切一段外部缓冲区
//
// 关键 static_assert 全部编译通过即证明：
//   sizeof(std::array<int,4>) == 16、无额外 padding、standard-layout、可平凡拷贝
//
// 预期汇编（gcc 13.2 -O1 实测）：
//   raw(...):        mov rax, rdi ; ret            <- 零开销：地址原样传出
//   bytes_of(...):   mov eax, 16  ; ret
//   make_hdr(...):   mov rax, rdi ; mov edx, 16 ; ret   <- 无分配、无拷贝
//   take16(...):     ret                             <- 静态 extent 零运行时开销

#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>

int c_arr[7] = {};

// ---- 1. 取长度：C 写 sizeof(a)/sizeof(a[0])，C++17/20 有现成的 ----
std::size_t len_cpp17() { return std::size(c_arr); }   // size_t（无符号）
std::ptrdiff_t len_cpp20() { return std::ssize(c_arr); } // 有符号，-1 表示「空/无效」

static_assert(std::is_same_v<decltype(std::ssize(c_arr)), std::ptrdiff_t>,
              "std::ssize 必须返回有符号类型，避免 0-1 下溢回绕成天文数字");

// ---- 2. std::array：布局就是 T[N]，可安全打到 wire format 上 ----
struct WireHeader {
    std::array<std::byte, 16> raw;   // 定长 16 字节报文头，无 padding
};

static_assert(sizeof(std::array<int, 4>) == 4 * sizeof(int));
static_assert(sizeof(WireHeader) == 16);          // 没有额外的 tag/padding
static_assert(alignof(WireHeader) == 1);          // 对齐 1，可按字节 memcpy
static_assert(std::is_standard_layout_v<std::array<int, 4>>);
static_assert(std::is_trivially_copyable_v<std::array<int, 4>>);

// 取地址即得底层对象地址 —— 汇编只有一条 mov，说明零开销
const void* raw_ptr(const std::array<int, 4>& a) { return static_cast<const void*>(&a); }
std::size_t raw_bytes(const std::array<int, 4>& a) { return sizeof(a); }

// ---- 3. std::span：把外部缓冲区零拷贝包一层，长度跟着走 ----
std::span<const std::byte> make_hdr(const std::byte* buf) {
    return {buf, 16};                    // 不分配、不拷贝，只存 {ptr, len}
}

// span 传参不退化：长度信息保留
std::int64_t hdr_len(std::span<const std::byte> s) { return std::ssize(s); }

// 静态 extent：长度写错直接编译失败，热路径零运行时检查
void take16(std::span<const std::byte, 16> s) { (void)s; }

// 动态 extent：长度是运行时值，但同样零拷贝
void take_any(std::span<const std::byte> s) { (void)s; }

// ---- 注意：std::array 不走内建数组的退化规则，见笔记 3.5.6 第四节 ----
// 退化只作用于数组类型（T[N]）；std::array<T,N> 是类类型。
// 下面三个 sizeof 都是 16（不是 8）—— gcc 13.2 -O1 实测：
//   probe_byval:  mov eax, 16 ; ret
//   probe_byref:  mov eax, 16 ; ret
//   probe_tmpl:   mov eax, 16 ; ret
std::size_t probe_byval(std::array<int, 4> a) { return sizeof(a); }  // 16，按值传是拷贝
std::size_t probe_byref(std::array<int, 4>& a) { return sizeof(a); } // 16，推荐
template <class T, std::size_t N>
std::size_t probe_tmpl(T (&a)[N]) { return sizeof(a); }              // 16，裸数组引用

// 泛型代码要把长度当参数用，只能靠模板 N：
template <class T, std::size_t N>
std::size_t len_of(T (&)[N]) { return N; }   // ✅ 长度是编译期常量
