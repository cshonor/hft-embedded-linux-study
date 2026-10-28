// log-macro-demo.cpp —— TheCherno #55「C++ 的宏」条件编译日志：完整可跑版
// 落在 Effective C++ Item02「仍可能需要预处理器的情况」下。
//
// ===== 三种编译方式（都能编过，不会出现 LOG 未定义）=====
//   clang++ -std=c++17 -DPR_DEBUG=1   -o demo log-macro-demo.cpp   # Debug：打日志
//   clang++ -std=c++17 -DPR_RELEASE=1 -o demo log-macro-demo.cpp   # Release：零开销
//   clang++ -std=c++17                -o demo log-macro-demo.cpp   # 都不定义：走 #else 兜底
//
// ===== 看预处理展开（强烈建议跑一次）=====
//   clang++ -std=c++17 -DPR_RELEASE=1 -E log-macro-demo.cpp | tail -20
//   → main 里 LOG(...) 整行消失，只剩空语句 ';'
//
// ===== CMake（按构建类型自动切换，不用手改代码）=====
//   target_compile_definitions(demo PRIVATE
//       $<$<CONFIG:Debug>:PR_DEBUG=1>
//       $<$<CONFIG:Release>:PR_RELEASE=1>)
//
// ===== Visual Studio =====
//   项目属性 → C/C++ → 预处理器 → 预处理器定义：加 PR_DEBUG=1
//
// 注意统一写 -DPR_DEBUG=1，别依赖 -DPR_DEBUG 的"默认=1"行为（部分嵌入式工具链不保证）。

#include <cassert>
#include <iostream>
#include <string>

// ---------- 原始版（TheCherno 视频里那样写，缺 #else 兜底）----------
// 两个分支都不成立时 LOG 根本没定义 → error: use of undeclared identifier 'LOG'
//   #if PR_DEBUG == 1
//   #define LOG(x) std::cout << x << std::endl
//   #elif defined(PR_RELEASE)
//   #define LOG(x)
//   #endif

// ---------- 加固版：#else 兜底 + '\n' 代替 std::endl + 变参宏 ----------
// 变参日志的**正确写法**：宏只负责「要不要调用」，拼接交给类型安全函数。
// 直接写 #define LOGF(...) std::cout << __VA_ARGS__ << '\n' 是错的——
// 展开后 "a=", 1 里的逗号变成**逗号运算符**，输出和优先级全乱（实测会打出一串垃圾）。
template <class... Args>
void log_line(const char* tag, Args&&... args) {
    ((std::cout << tag) << ... << args) << '\n';   // C++17 二元左折叠（init 必须加括号）
}

#if PR_DEBUG == 1
#define LOG_MODE "DEBUG"
#define LOG(x)    std::cout << "[D] " << x << '\n'
#define LOGF(...) log_line("[D] ", __VA_ARGS__)
#elif defined(PR_RELEASE)
#define LOG_MODE "RELEASE"
#define LOG(x)                 // 空宏：实参完全不求值
#define LOGF(...)              // 同上
#else
#define LOG_MODE "DEFAULT (log off)"
#define LOG(x)
#define LOGF(...)
#endif

int main() {
    std::cout << "build mode : " << LOG_MODE << '\n';

    LOG("Hello");                     // Debug 打印；Release 整行消失
    LOGF("a=", 1, " b=", 2);          // 变参宏：裸逗号不再报 too many arguments

    // 坑 4：空宏把实参整个吞掉 → Debug/Release 行为不一致
    int n = 0;
    LOG(++n);                         // 铁律：日志实参绝不能有副作用
    std::cout << "n           = " << n << '\n';   // Debug: 1；Release/默认: 0

    // 同类机制 assert / NDEBUG（独立于 PR_DEBUG，用 -DNDEBUG 才关）
    int m = 0;
    assert(++m == 1);                 // NDEBUG 下展开成 ((void)0)，++m 不求值
    std::cout << "m           = " << m << '\n';   // 默认: 1；-DNDEBUG: 0

    return 0;
}
