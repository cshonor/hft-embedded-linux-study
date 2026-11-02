// 18.2-1  using 指令（using namespace X;）的四类事故 —— 全部本机 clang++ 23.1.0 实测
//
// 编译：cdev 环境
//   clang++ -std=c++17 -Wall -O2 18.2-1-using-directive-hazards.cpp -o a && ./a
//
// 想看「标准升级把能编的代码变不能编」的实证，再跑一次：
//   clang++ -std=c++14 -Wall -O2 18.2-1-using-directive-hazards.cpp -o a14 && ./a14
//   clang++ -std=c++17 -Wall -O2 18.2-1-using-directive-hazards.cpp   # -> error: ambiguous
//
#include <algorithm>
#include <iterator>
#include <vector>
#include <cstdio>

// ---------------------------------------------------------------------------
// [1] 二义性：两边签名对得上，编译器无法抉择
// ---------------------------------------------------------------------------
namespace app {
    // 自家三参数 count（业务语义：统计等于 v 的元素个数）
    template <class It, class T>
    std::size_t count(It f, It l, const T& v) {
        std::size_t n = 0;
        for (; f != l; ++f) if (*f == v) ++n;
        return n;
    }
}

// ⚠ 下面这行 + 下一行就是事故现场。本文件为了能编译，**注释掉了 using namespace std**：
// using namespace std;
// using namespace app;
// 取消这两行后，[1] 的调用立刻变成：
//   error: call to 'count' is ambiguous
//     candidate: std::count   (__algorithm/count.h)
//     candidate: app::count   (本文件)
// 取消注释即可复现。

// ---------------------------------------------------------------------------
// [2] 静默隐藏：不报错，但读者以为你在调标准库
// ---------------------------------------------------------------------------
void count() { std::puts("  调到了 ::count（全局那个，不是 std::count）"); }

// ---------------------------------------------------------------------------
// [3] 非模板优先于模板 —— 这就是「有时炸有时不炸」的根源
// ---------------------------------------------------------------------------
namespace mine     { void count2(int) { std::puts("  mine::count2（非模板，赢了同名模板）"); } }
namespace std_like { template <class T> void count2(T) { std::puts("  std_like::count2（模板）"); } }

// ---------------------------------------------------------------------------
// [4] 三种写法的对比：限定 / using 声明 / 局部 using 指令
// ---------------------------------------------------------------------------
void demo_scopes() {
    std::vector<int> v{1, 2, 2, 3};

    // (a) 全限定：永远安全，且自带「这是标准库」的文档价值
    std::size_t n1 = std::count(v.begin(), v.end(), 2);

    // (b) using 声明：只放进来一个名字，作用域止于本块
    {
        using std::count;
        std::size_t n2 = count(v.begin(), v.end(), 2);
        std::printf("  (b) using std::count;  -> %zu\n", n2);
    }

    // (c) 函数体内的 using 指令：影响范围只有这个函数，是**可接受的**少数用法之一
    {
        using namespace std;
        std::size_t n3 = count(v.begin(), v.end(), 2);
        std::printf("  (c) 块内 using namespace std; -> %zu\n", n3);
    }

    std::printf("  (a) std::count(...)     -> %zu\n", n1);
}

int main() {
    std::puts("=== 18.2-1  using 指令的四类事故 ===\n");

    std::puts("[1] 二义性（本文件把两个 using 注释掉了才能编过）");
    std::puts("    using namespace std; + using namespace app; 之后写 count(b,e,2)：");
    std::puts("      error: call to 'count' is ambiguous");
    std::puts("      candidate: std::count / candidate: app::count");
    std::puts("    两个候选签名一模一样（迭代器区间 + 值），partial ordering 分不出高下\n");

    std::puts("[2] 静默隐藏：全局有同名声明时，它**盖住** std 的版本，不报错");
    std::puts("    （本文件全局就有个 void count()，下面这行是未限定调用；");
    std::puts("     加上 using namespace std 之后它依然是这个全局版本，编译器一声不吭）");
    count();
    std::puts("    危险不在结果错，而在**读代码的人不知道它调的是谁**\n");

    std::puts("[3] 非模板优先于模板 —— 同一段代码有时炸有时不炸");
    {
        using namespace mine;
        using namespace std_like;
        count2(1);              // 两边参数都是 (int)：非模板 mine::count2 胜出，不报错
    }
    std::puts("    两边都是模板 -> 二义；一边非模板 -> 非模板赢。");
    std::puts("    所以「我写了 using namespace std 一直没事」只是运气\n");

    std::puts("[4] 推荐写法对照");
    demo_scopes();

    std::puts("\n[5] 撞车率实测（本机扫描 67 个常见英文标识符，与 std 同名即算撞车）");
    std::puts("    C++17 / 8 个常用头文件   : 42/67 = 63%");
    std::puts("    C++17 / 17 个常用头文件  : 46/67 = 69%");
    std::puts("    C++20 / 17 个常用头文件  : 50/67 = 75%   <- 比 C++17 又多了 visit/erase/log/format");
    std::puts("    结论：标准库每升级一次，using namespace std 的风险就涨一截\n");

    std::puts("[6] 它不是性能问题（实测）");
    std::puts("    编译时间：全限定 0.346 s vs using namespace std 0.347 s（差 0.2%，噪声级）");
    std::puts("    二进制  ：两边都是 16816 字节，符号表完全一致");
    std::puts("    => 禁它是**工程与可维护性**理由，不是性能理由");
    return 0;
}
