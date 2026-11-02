// 18.2-4  「今天能编的代码，明天不能编」——标准库升级会主动来找你麻烦
//
// 这个文件的默认路径是安全的（TRIAL=0），但把 TRIAL 改成 1 就能看到版本脆弱性：
//
//   clang++ -std=c++14 -DTRIAL=1 -Wall 18.2-4-version-fragility.cpp -o a14   # ✅ 编过
//   clang++ -std=c++17 -DTRIAL=1 -Wall 18.2-4-version-fragility.cpp -o a17   # ❌ ambiguous
//
// 原因：std::size 是 C++17 才加入 <iterator> 的。项目从 C++14 升到 C++17 的那天，
//       app::size 突然多了一个平级对手 std::size，签名还一模一样。
//
#include <vector>
#include <cstdio>
#if __cplusplus >= 201703L
#include <iterator>          // C++17 起这里才有 std::size
#endif

#ifndef TRIAL
#define TRIAL 0              // 改成 1 复现「升标准即编译失败」
#endif

namespace app {
    // 自家 size：返回 int（有符号），和 std::size 的返回类型不同，但签名结构相同
    template <class C> int size(const C& c) { return (int)c.size(); }
}

int main() {
    std::vector<int> v{1, 2, 3};
    std::printf("=== 18.2-4  版本脆弱性（__cplusplus = %ld）===\n\n", (long)__cplusplus);

#if TRIAL
    using namespace std;
    using namespace app;
    std::printf("  size(v) = %d\n", size(v));    // C++14: OK  C++17: error: ambiguous
#else
    std::printf("  app::size(v) = %d   （全限定，任何标准版本都稳）\n", app::size(v));
#if __cplusplus >= 201703L
    std::printf("  std::size(v) = %zu\n", std::size(v));
#endif
#endif

    std::puts("\n实测记录（本机 clang++ 23.1.0，libc++）：");
    std::puts("  -std=c++14 -DTRIAL=1 : 编译通过，输出 3");
    std::puts("  -std=c++17 -DTRIAL=1 : error: call to 'size' is ambiguous");
    std::puts("                         candidate: std::size(const _Cont&)   [__iterator/size.h]");
    std::puts("                         candidate: app::size(const C&)");
    std::puts("\n同类风险名单（每次标准升级都在变长）：");
    std::puts("  C++11 -> std::begin/end/next/prev/move/forward");
    std::puts("  C++17 -> std::size/empty/data/clamp/sample  （size 是头号杀手）");
    std::puts("  C++20 -> std::ssize/ranges::count/ranges::find/erase/erase_if");
    std::puts("\n结论：using namespace std 让你的代码**依赖标准库的具体版本**。");
    std::puts("      全限定名把这个依赖切断——库加多少新名字都动不了你。");
    return 0;
}
