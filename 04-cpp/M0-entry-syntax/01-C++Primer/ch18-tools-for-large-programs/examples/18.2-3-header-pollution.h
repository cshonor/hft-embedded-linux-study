// 18.2-3  头文件里的 using 指令会传染每一个包含者
//
// 这是 18.2-3-header-pollution.cpp 依赖的「坏头文件」。
// 它只做了一件错事：在全局作用域写了 using namespace std。
//
// 编译：clang++ -std=c++17 -Wall -O2 18.2-3-header-pollution.cpp -o a && ./a
//
#pragma once
#include <algorithm>

// ❌ 事故源头：头文件里的 using 指令。
//    它不属于这个头文件——它会跟着 #include 扩散到**每一个**包含者的全局作用域，
//    而那些 TU 的作者根本不知道自己被污染了。
using namespace std;

namespace util {
    inline int clamp01(int x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }
}
