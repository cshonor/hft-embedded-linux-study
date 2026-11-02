// 18.2-2  ADL 与 using 指令的相互作用：为什么「两步走」不能用 using namespace 替代
//
// 编译：clang++ -std=c++17 -Wall -O2 18.2-2-adl-two-step.cpp -o a && ./a
//
// 把下面 good() 里的 `using std::swap;` 换成 `using namespace std;` 即可复现二义：
//   error: call to 'swap' is ambiguous
//     candidate: md::swap(S&, S&)      <- ADL 找来的
//     candidate: ::swap(md::S&, md::S&) <- 外层遗留的全局 swap
//
#include <utility>
#include <cstdio>

namespace md {
    struct Order { int qty; char pad[64]; };          // 故意做大：自定义 swap 才有意义
    void swap(Order& a, Order& b) {
        Order t = a; a = b; b = t;
        std::puts("  -> md::swap（自定义）");
    }
}

// 遗留代码里的全局 swap（老 C 风格库、或某次手滑把函数放到了全局）
void swap(md::Order& a, md::Order& b) { std::puts("  -> ::swap（遗留全局，多半不是你想要的）"); }

// ---------- 写法 A：两步走（using 声明）—— 正确 ----------
template <class T>
void good(T& a, T& b) {
    using std::swap;     // ① 把 std::swap 放进本块作用域 —— 它**隐藏**了外层的 ::swap
    swap(a, b);          // ② 未限定调用：候选 = std::swap + ADL 找来的 md::swap
                         //    md::swap 是非模板精确匹配，胜出；int 之类没有自定义 swap 时落到 std::swap
}

// ---------- 写法 B：using 指令 —— 会炸 ----------
// template <class T>
// void bad(T& a, T& b) {
//     using namespace std;   // 不隐藏外层！查找集 = ::swap + std::swap + ADL(md::swap) 三方混战
//     swap(a, b);            // ::swap 和 md::swap 签名完全相同 -> error: ambiguous
// }

int main() {
    std::puts("=== 18.2-2  ADL 两步走 vs using namespace ===\n");

    md::Order x{1, {}}, y{2, {}};
    std::puts("[A] using std::swap; swap(a, b);   // 推荐");
    good(x, y);
    std::puts("    using 声明把 std::swap 放进块作用域，同时**遮住**外层遗留的 ::swap，");
    std::puts("    候选集只剩 std::swap + ADL 命中的 md::swap，非模板精确匹配胜出\n");

    int p = 3, q = 4;
    std::puts("[B] 没有自定义 swap 的类型（int）");
    good(p, q);
    std::printf("    p=%d q=%d  -> 落到 std::swap\n", p, q);

    std::puts("\n[C] 换成 using namespace std 会发生什么");
    std::puts("    error: call to 'swap' is ambiguous");
    std::puts("      candidate: md::swap(md::Order&, md::Order&)");
    std::puts("      candidate: ::swap(md::Order&, md::Order&)");
    std::puts("    原因：using 指令**不隐藏**任何名字，它只是让 std 的名字「看起来也在」外层作用域，");
    std::puts("    于是外层遗留的 ::swap 和 ADL 找来的 md::swap 平级竞争，谁也赢不了\n");

    std::puts("要点：");
    std::puts("  - using 声明（using std::swap;）  = 声明，会隐藏外层同名");
    std::puts("  - using 指令（using namespace std;）= 不隐藏，只把名字「摊平」到公共作用域");
    std::puts("  - 泛型代码里写 swap/begin/end/operator<< 一律用两步走，别用 using namespace");
    return 0;
}
