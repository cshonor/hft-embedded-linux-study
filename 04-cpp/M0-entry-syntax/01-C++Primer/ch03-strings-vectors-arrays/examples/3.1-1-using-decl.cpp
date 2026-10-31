// 3.1 —— using 声明 / using 指示 / ADL 实测
// 编译：clang++ -std=c++17 -O2 examples/3.1-1-using-decl.cpp -o using && ./using
// 把下面标了 ❌ 的行解除注释，可以亲自看到三条真实报错。
#include <cstdio>
#include <string>
#include <utility>

namespace lib_a { void log(const std::string& s) { printf("  [lib_a] %s\n", s.c_str()); } }
namespace lib_b { void log(const std::string& s) { printf("  [lib_b] %s\n", s.c_str()); } }

namespace orderbook {
    struct Book { int id = 0; };
    void swap(Book& a, Book& b) {
        printf("  → 命中 orderbook::swap（ADL 找到的定制版）\n");
        int t = a.id; a.id = b.id; b.id = t;
    }
}

int main() {
    printf("① using 声明：只引入指定的那一个名字\n");
    {
        using lib_a::log;
        log("只引入 lib_a::log，没有歧义");
    }

    printf("② using 指示：引入整个命名空间，同名就炸\n");
    {
        using namespace lib_a;
        using namespace lib_b;
        // log("x");                 // ❌ error: call to 'log' is ambiguous
        lib_a::log("只能靠显式限定消歧");
        lib_b::log("只能靠显式限定消歧");
        printf("  放开上面那行注释 → error: call to 'log' is ambiguous\n");
    }

    printf("③ using 声明能覆盖 using 指示带来的名字\n");
    {
        using namespace lib_a;
        using namespace lib_b;
        using lib_a::log;              // 显式点名，歧义消失
        log("走 lib_a::log");
    }

    printf("④ 函数内的 using namespace：作用域只到函数结束\n");
    {
        using namespace lib_a;
        log("函数内可见");
    }
    // log("函数外不可见");            // ❌ error: use of undeclared identifier 'log'

    printf("⑤ ADL：实参所在命名空间的函数会被自动找到\n");
    {
        orderbook::Book x{1}, y{2};
        using std::swap;               // 兜底：标准库版本
        swap(x, y);                    // 实参在 orderbook → ADL 命中 orderbook::swap
        printf("  x.id=%d y.id=%d\n", x.id, y.id);
        int m = 1, n = 2;
        swap(m, n);                    // int 不在任何命名空间 → 靠 using std::swap
        printf("  int: m=%d n=%d\n", m, n);
    }

    printf("⑥ 两个 using 声明指向同名 → 编译期就拦下（比 using 指示安全）\n");
    printf("  同一作用域写 using lib_a::log; using lib_b::log;\n"
           "  → error: target of using declaration conflicts with declaration already in scope\n");
    return 0;
}
