// 3.5.6-3-initializer-list-trap.cpp
// 编译：g++ -std=c++20 3.5.6-3-initializer-list-trap.cpp
// 实测环境：Compiler Explorer / g132（gcc 13.2.0, x86-64 linux）
//
// 目的：验证真正会咬人的那处差异——auto a = {1,2,3} 得到的不是数组。
//
// gcc 13.2 汇编实证（-O1）：
//   a:
//           .quad   reference temporary for a      <- 符号名自曝：这是「a 的临时对象」
//           .quad   3                              <- 长度
//   reference temporary for a:
//           .long   1
//           .long   2
//           .long   3                              <- 底层数组，生命周期到语句结束
//
//   get_a(unsigned):  mov rax, QWORD PTR a[rip]    <- 要先取「临时数组指针」
//                     mov eax, DWORD PTR [rax+rdi*4]
//   get_b(unsigned):  mov eax, DWORD PTR b[0+rdi*4] <- std::array 一次寻址
//
// 即 auto 版本每次访问多一次 load。

#include <array>
#include <initializer_list>
#include <type_traits>

// ❌ 危险：auto 推导出 std::initializer_list<int>，不是数组
auto a = {1, 2, 3};

// ✅ 这才是聚合类型 std::array<int, 3>
std::array b{1, 2, 3};

static_assert(std::is_same_v<decltype(a), std::initializer_list<int>>,
              "auto + 花括号 => initializer_list，不是数组");
static_assert(std::is_same_v<decltype(b), std::array<int, 3>>);

// initializer_list 只存 {首元素指针, 长度} 两个机器字，不延长底层数组寿命。
// 上面 a 之所以「看起来能用」，是因为它是全局对象 —— 底层数组和 a 同为静态存储期。
// 函数内的 auto a = {1,2,3} 则不同：语句一结束，底层数组就没了，a 里的指针悬垂。

// 危险示范（默认注释掉，取消注释即可看到编译器报错）：
// auto make() { return {1, 2, 3}; }
//   error: returning initializer list

// 即使显式写返回类型，GCC 仍会警告底层数组活不过返回值：
std::initializer_list<int> make3() {
    return {1, 2, 3};
    // gcc 13.2 warning:
    //   returning temporary 'initializer_list' does not extend the lifetime
    //   of the underlying array [-Winit-list-lifetime]
}

// ✅ 正确写法：返回 std::array，汇编只有两次 mov + ret
std::array<int, 3> make2() { return {1, 2, 3}; }

// ❌ 更隐蔽的坑：auto + 单元素花括号，类型推导成 initializer_list<int>
//    HFT 代码里写 auto buf = {0}; 然后当数组用 —— 典型 UB
auto zero = {0};
static_assert(std::is_same_v<decltype(zero), std::initializer_list<int>>);

// 修复：显式类型，或用 C 风格数组（如果只要一个值，直接写 int zero = 0;）
int zero_fixed = 0;
