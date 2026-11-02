// 18.2-3  我只包含了一个工具头，从没写过 using namespace std —— 但我被污染了
//
// 编译（默认 SAFE 路径，能编过）：
//   clang++ -std=c++17 -Wall -O2 18.2-3-header-pollution.cpp -o a && ./a
//
// 想看事故：把下面的 #define TRY_UNQUALIFIED 改成 1 重新编译：
//   error: call to 'count' is ambiguous
//     candidate: std::count    （来自头文件里的 using namespace std）
//     candidate: app::count    （本文件自己写的）
//   注意：报错行在本文件，但**罪魁在 util 那个头文件里**。
//
#include "18.2-3-header-pollution.h"
#include <vector>
#include <cstdio>

#define TRY_UNQUALIFIED 0        // <- 改成 1 复现二义

namespace app {
    // 业务代码自己写的三参数 count
    template <class It, class T>
    std::size_t count(It f, It l, const T& v) {
        std::size_t n = 0;
        for (; f != l; ++f) if (*f == v) ++n;
        return n;
    }
}

int main() {
    std::puts("=== 18.2-3  头文件污染的传染 ===\n");

    std::vector<int> v{1, 2, 2, 3};

#if TRY_UNQUALIFIED
    // ❌ 未限定调用：std::count（被头文件塞进全局）和 app::count 平级竞争 -> ambiguous
    using namespace app;
    std::printf("%zu\n", count(v.begin(), v.end(), 2));
#else
    // ✅ 全限定：谁的就是谁的，不受任何 using 指令影响
    std::printf("  app::count(v.begin(), v.end(), 2) = %zu\n", app::count(v.begin(), v.end(), 2));
    std::printf("  std::count(v.begin(), v.end(), 2) = %zu\n", std::count(v.begin(), v.end(), 2));
#endif

    std::puts("\n本 TU 的关键事实：");
    std::puts("  1. 我从未写过 using namespace std");
    std::puts("  2. 我也从未 #include <algorithm>");
    std::puts("  3. 但只要 #include 了那个坏头文件，std 的几百个名字就已经在我的全局作用域里了");
    std::puts("\n这就是 C++ Core Guidelines SF.7 说的：");
    std::puts("  \"Doing so takes away an #includer's ability to effectively disambiguate\"");
    std::puts("  \"and to use alternatives. It also makes #include'd headers order-dependent.\"");
    std::puts("\n纪律：头文件里只允许 using 声明，且最好连声明都不要；using 指令一律禁止。");

    std::printf("\n（顺带演示 util 头本身的功能正常：clamp01(-5) = %d）\n", util::clamp01(-5));
    return 0;
}
