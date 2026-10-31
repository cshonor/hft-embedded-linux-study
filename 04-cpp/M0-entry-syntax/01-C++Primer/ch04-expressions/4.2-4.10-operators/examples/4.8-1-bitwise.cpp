// 4.8 位运算符：优先级、移位边界、UB、popcount、寄存器操作
//
// 编译：clang++ -std=c++17 -O2 -Wall 4.8-1-bitwise.cpp -o bit && ./bit
//       （会看到两条预期内的警告：-Wparentheses 与 -Wshift-count-overflow）
#include <chrono>
#include <cstdint>
#include <bitset>
#include <cstdio>

// ---------- ① 优先级坑：& 比 == 低 ----------
static int prio(int x)    { return x & 0xFF == 0; }     // 实际是 x & (0xFF == 0)
static int prio_ok(int x) { return (x & 0xFF) == 0; }   // 本意：低 8 位全 0

// ---------- ② 移位的三种边界 ----------
static int      shift_neg()    { int v = -8; return v >> 1; }
static int      shift_ub1()    { return 1 << 31; }                     // int 左移进符号位：UB
static unsigned shift_ub2()    { unsigned u = 1u; return u << 32; }    // 移位量 >= 位宽：UB
static unsigned shift_ok()     { unsigned u = 1u; return u << 31; }

// ---------- ③ 位运算 vs 乘除 ----------
static int mul4(int x) { return x * 4; }
static int shl2(int x) { return x << 2; }
static int div8(int x) { return x / 8; }   // 有符号除法向零取整
static int shr3(int x) { return x >> 3; }  // 算术右移向下取整

// ---------- ⑧ 异或交换（经典技巧，但有陷阱） ----------
static void xor_swap(unsigned& a, unsigned& b) { a ^= b; b ^= a; a ^= b; }

// ---------- ④ popcount ----------
static int popcount_loop(uint64_t x) { int c = 0; while (x) { c += (int)(x & 1); x >>= 1; } return c; }

int main() {
    printf("① 优先级：& 低于 ==\n");
    int x = 0x100;
    printf("  x & 0xFF == 0    → %d（被解析成 x & (0xFF == 0)）\n", prio(x));
    printf("  (x & 0xFF) == 0  → %d（x=0x100 低 8 位为 0 → 真）\n", prio_ok(x));

    printf("② 移位的边界\n");
    printf("  -8 >> 1    = %d      （算术右移，补符号位；标准说实现定义）\n", shift_neg());
    printf("  1 << 31    = %d  （int 左移进符号位 = UB）\n", shift_ub1());
    printf("  1u << 32   = %u  （移位量 >= 32 = UB，警告 -Wshift-count-overflow）\n", shift_ub2());
    printf("  1u << 31   = %u       （用 unsigned 才是正确写法）\n", shift_ok());

    printf("③ 位运算 vs 乘除（负数上不等价！）\n");
    printf("  x=100 : x*4=%d, x<<2=%d, x/8=%d, x>>3=%d\n", mul4(100), shl2(100), div8(100), shr3(100));
    printf("  x=-100: x/8=%d, x>>3=%d   ← / 向零取整，>> 向下取整\n", div8(-100), shr3(-100));

    printf("④ popcount（1000 万次）\n");
    const int N = 10'000'000;
    uint64_t v = 0x123456789ABCDEFULL;
    volatile int sink = 0;
    {
        auto t0 = std::chrono::steady_clock::now();
        int c = 0;
        for (int i = 0; i < N; ++i) c += __builtin_popcountll(v + i);
        auto t1 = std::chrono::steady_clock::now();
        printf("  __builtin_popcountll : %6.1f ms (c=%d)\n",
               std::chrono::duration<double, std::milli>(t1 - t0).count(), c);
        sink = c;
    }
    {
        auto t0 = std::chrono::steady_clock::now();
        int c = 0;
        for (int i = 0; i < N; ++i) c += popcount_loop(v + i);
        auto t1 = std::chrono::steady_clock::now();
        printf("  手写循环             : %6.1f ms (c=%d)\n",
               std::chrono::duration<double, std::milli>(t1 - t0).count(), c);
        sink = c;
    }

    printf("⑤ 寄存器操作三件套（嵌入式最常用）\n");
    uint32_t reg = 0x0000;
    reg |=  (1u << 3);  printf("  置位 bit3 → 0x%08X\n", reg);
    reg &= ~(1u << 3);  printf("  清位 bit3 → 0x%08X\n", reg);
    reg = 0xA5;
    printf("  读位 bit2 → %u\n", (reg >> 2) & 1u);
    reg ^= (1u << 0);   printf("  翻转 bit0 → 0x%08X\n", reg);

    printf("⑥ 有符号 vs 无符号\n");
    int s = -1;
    printf("  (int)-1 的位模式 0x%08X，~0 = %d\n", (unsigned)s, ~0);

    printf("⑦ std::bitset<64> vs 手写 uint64_t（1000 万次 set/reset/test）\n");
    {
        const int M = 10'000'000;
        volatile uint64_t vsink = 0;
        uint64_t flags = 0;
        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < M; ++i) {
            flags |=  (1ull << (i & 63));
            flags &= ~(1ull << ((i + 7) & 63));
            if (flags & (1ull << 5)) vsink += 1;
        }
        auto t1 = std::chrono::steady_clock::now();
        double ms_raw = std::chrono::duration<double, std::milli>(t1 - t0).count();

        std::bitset<64> bs;
        auto t2 = std::chrono::steady_clock::now();
        for (int i = 0; i < M; ++i) {
            bs.set(i & 63);
            bs.reset((i + 7) & 63);
            if (bs.test(5)) vsink += 1;
        }
        auto t3 = std::chrono::steady_clock::now();
        double ms_bs = std::chrono::duration<double, std::milli>(t3 - t2).count();
        printf("  uint64_t + 位运算 : %6.1f ms\n", ms_raw);
        printf("  std::bitset<64>   : %6.1f ms  (%.2fx —— 同量级，互有胜负)\n", ms_bs, ms_bs / ms_raw);
    }

    printf("⑧ 异或交换：不快，而且有自交换陷阱\n");
    {
        unsigned a = 42, b = 42;
        xor_swap(a, b);
        printf("  同值不同对象 swap(42,42) -> a=%u b=%u  （没问题）\n", a, b);
        unsigned u = 42;
        xor_swap(u, u);                 // 同一个对象 → 归零
        printf("  同一对象 swap(u,u)       -> u=%u       ← 真陷阱\n", u);
    }

    printf("⑨ 常用位技巧\n");
    uint64_t w = 0xF0F0F0F0F0F0F0F0ULL;
    int cnt = 0;
    while (w) { w &= (w - 1); ++cnt; }  // 每次消掉最低位的 1
    printf("  x &= (x-1) 数 1 位：0xF0F0...F0 有 %d 个 1（循环 %d 次，不是 64 次）\n", cnt, cnt);
    printf("  判 2 的幂 n && !(n & (n-1))：8 -> %d，7 -> %d\n",
           !!(8 && !(8 & (8 - 1))), !!(7 && !(7 & (7 - 1))));
    return 0;
}
