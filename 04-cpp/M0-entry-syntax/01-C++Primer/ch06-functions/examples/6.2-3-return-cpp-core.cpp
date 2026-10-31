// 6.2-3-return-cpp-core.cpp —— 返回值的 ABI：sret、RVO、以及"返回什么"的工程选择
//
// 编译运行：
//   clang++ -std=c++17 -Wall -O2 6.2-3-return-cpp-core.cpp -o a && ./a
// 看返回大对象的隐藏指针（AAPCS 用 x8 传 sret 指针）：
//   clang++ -std=c++17 -O0 -S 6.2-3-return-cpp-core.cpp -o - | grep -E "memcpy|x8" | head
//
// ⚠ 本文件里 4 处 warning 是**故意演示**的（返回栈对象引用/指针、pessimizing move），
//   它们正是本节要讲的错误写法。

#include <chrono>
#include <cstdio>
#include <optional>
#include <string>
#include <utility>

// ---------- 0. 计数型探针：拷贝/移动到底发生了几次（比计时更确定） ----------
struct Tracker {
    static int ctors, copies, moves;
    int v;
    explicit Tracker(int x = 0) : v(x) { ++ctors; }
    Tracker(const Tracker& o) : v(o.v) { ++copies; }
    Tracker(Tracker&& o) noexcept : v(o.v) { ++moves; }
};
int Tracker::ctors = 0, Tracker::copies = 0, Tracker::moves = 0;
static void reset() { Tracker::ctors = Tracker::copies = Tracker::moves = 0; }

__attribute__((noinline)) Tracker ret_plain(int x) {
    Tracker t(x);
    return t;                          // ✅ NRVO：直接在调用方内存上构造
}
__attribute__((noinline)) Tracker ret_moved(int x) {
    Tracker t(x);
    return std::move(t);               // ⚠ -Wpessimizing-move：move 阻止 NRVO
}
__attribute__((noinline)) Tracker ret_temp(int x) {
    return Tracker(x);                 // ✅ 纯右值，C++17 起直接在目标位置构造
}

// ---------- 1. 大对象返回：调用方把"接收地址"偷偷传给被调函数（sret） ----------
struct Big {
    char data[256];
    int  tag;
};
__attribute__((noinline)) Big make_big(int tag) {
    Big b{};
    b.tag = tag;
    b.data[0] = (char)tag;
    return b;
}

// ---------- 2. 返回引用/指针的生命周期 ----------
const int& bad_ref() {
    int x = 42;
    return x;                    // ⚠ -Wreturn-stack-address
}
const char* bad_ptr() {
    std::string s = "hello";
    return s.c_str();            // ⚠ 返回已析构对象的内部指针
}

// ---------- 3. "返回什么"的三选一：错误码 / optional / 异常 ----------
enum class Err { Ok, NotFound };
// (a) 错误码 + 出参：零开销，但调用方容易忘检查
__attribute__((noinline)) bool lookup_ec(int key, int& out) {
    if (key < 0) return false;
    out = key * 2;
    return true;
}
// (b) optional：值语义，强制检查（否则 UB）
__attribute__((noinline)) std::optional<int> lookup_opt(int key) {
    if (key < 0) return std::nullopt;
    return key * 2;
}
// (c) 异常：正常路径几乎零成本（zero-cost），错误路径昂贵
__attribute__((noinline)) int lookup_throw(int key) {
    if (key < 0) throw Err::NotFound;
    return key * 2;
}

int main() {
    printf("=== 6.2-3 返回类型：ABI、RVO、以及工程选择 ===\n\n");

    printf("[1] 大对象返回走隐藏指针（sret），不是真的「拷回来」\n");
    Big b1 = make_big(7);
    printf("    make_big(7).tag = %d，sizeof(Big) = %zu 字节\n", b1.tag, sizeof(Big));
    printf("    > 16 字节放不进寄存器 → 签名被改成 void make_big(Big* sret, int)\n\n");

    printf("[2] RVO 到底生效没：数拷贝/移动次数（比计时可靠）\n");
    reset(); { Tracker a = ret_plain(1); (void)a; }
    printf("    return t;          构造=%d 拷贝=%d 移动=%d\n",
           Tracker::ctors, Tracker::copies, Tracker::moves);
    reset(); { Tracker b = ret_moved(2); (void)b; }
    printf("    return std::move(t);构造=%d 拷贝=%d 移动=%d   ← 多出一次移动\n",
           Tracker::ctors, Tracker::copies, Tracker::moves);
    reset(); { Tracker c = ret_temp(3); (void)c; }
    printf("    return Tracker(x); 构造=%d 拷贝=%d 移动=%d   ← C++17 保证零拷贝\n",
           Tracker::ctors, Tracker::copies, Tracker::moves);
    printf("    结论：具名局部变量返回时**不要**加 std::move（-Wpessimizing-move 会教你）\n\n");

    printf("[3] 返回引用/指针指向已销毁对象（-Wall 会警告）\n");
    const int& r = bad_ref();
    printf("    bad_ref() 读到 %d —— 每次运行都可能不同，纯 UB\n", r);
    const char* p = bad_ptr();
    printf("    bad_ptr() 读到 \"%s\" —— std::string 已析构，同样是 UB\n\n", p);

    printf("[4] 错误处理三选一：各 100 万次（noinline，防止被折叠）\n");
    const int N = 1000000;
    volatile long acc = 0;

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) { int out; if (lookup_ec(i, out)) acc += out; }
    auto t1 = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) { auto o = lookup_opt(i); if (o) acc += *o; }
    auto t2 = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) { try { acc += lookup_throw(i); } catch (...) {} }
    auto t3 = std::chrono::steady_clock::now();
    long thrown = 0;
    for (int i = 0; i < N; ++i) {
        try { acc += lookup_throw(i % 100 == 0 ? -1 : i); }
        catch (...) { ++thrown; }
    }
    auto t4 = std::chrono::steady_clock::now();

    auto ns = [](auto a, auto b, long n) {
        return std::chrono::duration<double, std::nano>(b - a).count() / n;
    };
    double ec = ns(t0, t1, N), op = ns(t1, t2, N), ex = ns(t2, t3, N), ex2 = ns(t3, t4, N);
    printf("    (a) bool + 出参            成功路径 = %6.2f ns/次\n", ec);
    printf("    (b) std::optional<int>     成功路径 = %6.2f ns/次\n", op);
    printf("    (c) throw（全成功，不抛）   成功路径 = %6.2f ns/次\n", ex);
    printf("    (c') throw（1%% 真抛，%ld 次）平均   = %6.2f ns/次\n", thrown, ex2);
    printf("    单次抛出摊薄成本 ≈ %.0f ns —— 比成功路径高 2~3 个数量级\n",
           (ex2 - ex) * N / (thrown ? thrown : 1));
    printf("    acc = %ld\n\n", (long)acc);

    printf("[5] 一句话选择：热路径用错误码/optional；异常只留给「真的不该发生」的路径\n");
    return 0;
}

// ---------------------------------------------------------------------------
// 汇编侧证据（-O0 -S）：
//   make_big 的签名被改成 void make_big(Big* sret, int tag) —— 返回地址通过 x8 传入，
//   被调函数直接往那块内存写。这就是 C++ "返回大对象不拷贝"的真相。
//   NRVO 生效时：没有 memcpy；失效时（多个返回路径 / return std::move(t)）多一次移动。
