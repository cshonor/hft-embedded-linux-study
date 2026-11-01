// 4.8-1 位运算符：优先级、UB 边界、性能
//
// 编译：
//   clang++ -std=c++17 -O2 -Wall examples/4.8-1-bitwise.cpp -o bit && ./bit
//
// 注意：移位的两类 UB 只是「实测值」，换个编译器/平台就变，别依赖。

#include <bitset>
#include <chrono>
#include <cstdint>
#include <cstdio>

__attribute__((noinline)) static int popcount_naive(uint64_t x) {
    int n = 0;
    while (x) { n += (int)(x & 1u); x >>= 1; }
    return n;
}

int main() {
    printf("=== 4.8-1 位运算符实测 ===\n\n");

    // ---------- [1] & 低于 ==：测位最经典的错写 ----------
    printf("[1] 优先级：& 低于 ==\n");
    unsigned flags = 0b1010;
    printf("    flags = 0b1010\n");
    printf("    flags & 0b0010 == 0b0010       -> %d   // 实际是 flags & (0b0010==0b0010)\n",
           flags & 0b0010 == 0b0010 ? 1 : 0);
    printf("    (flags & 0b0010) == 0b0010     -> %d   // 加括号才对\n\n",
           (flags & 0b0010) == 0b0010 ? 1 : 0);

    // ---------- [2] 移位的三类边界 ----------
    printf("[2] 移位的边界（后两个是 UB，实测值仅供参考）\n");
    printf("    合法：1u << 3           = %u\n", 1u << 3);
    printf("    ❗ UB：1 << 32          = %d   // 移位量 >= 位宽（int 32 位）\n", 1 << 32);
    printf("    ❗ UB：-1 << 1          = %d   // 负数左移\n", -1 << 1);
    printf("    定义良好：-8 >> 1       = %d   // 负数右移是**实现定义**（算术右移保持符号）\n", -8 >> 1);
    printf("    安全写法：移位量先 & (width-1)，用无符号类型做位操作\n\n");

    // ---------- [3] 负数 /8 vs >>3：不等价 ----------
    printf("[3] 负数除法 vs 右移\n");
    int neg = -100;
    printf("    -100 / 8  = %d    // 向零取整\n", neg / 8);
    printf("    -100 >> 3 = %d    // 算术右移 = 向下取整（更负）\n", neg >> 3);
    printf("    正数时两者相同：100/8=%d, 100>>3=%d\n", 100 / 8, 100 >> 3);
    printf("    ⚠ 有符号数别用移位代替除法；编译器自己会把 /8 优化成移位\n\n");

    // ---------- [4] popcount：内建函数的价值 ----------
    printf("[4] popcount（1e6 次）\n");
    const int N = 1000000;
    volatile long long sink = 0;
    // 运行时才确定的数据：否则编译器会把「常量参数的纯函数」整个折叠掉，测出假结果
    static uint64_t vals[256];
    for (int k = 0; k < 256; ++k) vals[k] = 0x0123456789abcdefULL ^ (uint64_t)(k * 2654435761u);

    auto t0 = std::chrono::steady_clock::now();
    for (int k = 0; k < N; ++k) sink += popcount_naive(vals[k & 255]);
    auto t1 = std::chrono::steady_clock::now();

    auto t2 = std::chrono::steady_clock::now();
    for (int k = 0; k < N; ++k) sink += __builtin_popcountll(vals[k & 255]);
    auto t3 = std::chrono::steady_clock::now();

    double naive = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double builtin = std::chrono::duration<double, std::milli>(t3 - t2).count();
    printf("    手写循环          : %7.2f ms  (%.2f ns/次)\n", naive, naive * 1e6 / N);
    printf("    __builtin_popcountll: %7.2f ms  (%.2f ns/次)\n", builtin, builtin * 1e6 / N);
    printf("    比值 = %.1fx —— 编译器把它编成一条 cnt 指令\n\n",
           builtin > 0 ? naive / builtin : 0.0);

    // ---------- [5] bitset vs uint64_t：大小与速度 ----------
    printf("[5] std::bitset vs uint64_t\n");
    printf("    sizeof(std::bitset<64>) = %zu\n", sizeof(std::bitset<64>));
    printf("    sizeof(std::bitset<8>)  = %zu   // 按 8 字节对齐，不是 1 字节\n",
           sizeof(std::bitset<8>));
    printf("    sizeof(uint64_t)        = %zu\n", sizeof(uint64_t));
    std::bitset<8> bs("10100101");
    printf("    bitset<8>(\"10100101\").count() = %zu，to_ulong() = %lu\n",
           bs.count(), bs.to_ulong());
    printf("    取舍：bitset 可读性好、位数编译期固定；热路径用 uint64_t 手动位操作\n\n");

    // ---------- [6] 异或交换的自交换陷阱 ----------
    printf("[6] 异或交换的坑\n");
    int a = 5, b = 9;
    a ^= b; b ^= a; a ^= b;
    printf("    正常情况：a=5,b=9 -> a=%d, b=%d  （交换成功）\n", a, b);
    int s = 7;
    s ^= s;                      // 自己和自己异或 = 0
    printf("    自交换陷阱：s ^= s -> %d   // 同一个对象做异或交换会清零\n", s);
    printf("    ⚠ 别用异或交换：编译器对 std::swap 生成的代码更好，且没有这个坑\n\n");

    // ---------- [7] x & (x-1) 与判 2 的幂 ----------
    printf("[7] 位运算惯用法\n");
    auto is_pow2 = [](unsigned v) { return v != 0 && (v & (v - 1)) == 0; };
    unsigned x6 = 0b101100;
    printf("    x & (x-1) 清掉最低位的 1：%s -> %s\n",
           std::bitset<6>(x6).to_string().c_str(),
           std::bitset<6>(x6 & (x6 - 1)).to_string().c_str());
    printf("    判 2 的幂：1=%d 2=%d 3=%d 8=%d 0=%d\n",
           is_pow2(1), is_pow2(2), is_pow2(3), is_pow2(8), is_pow2(0));
    printf("    取最低位的 1：x & -x  -> %s -> %s\n",
           std::bitset<6>(x6).to_string().c_str(),
           std::bitset<6>(x6 & (~x6 + 1)).to_string().c_str());
    printf("    置位/清位/测位：x |= (1<<n) / x &= ~(1<<n) / (x & (1<<n)) != 0\n\n");

    // ---------- [8] 速查 ----------
    printf("[8] 速查\n");
    printf("    • 位运算只对**无符号**类型做（有符号的移位/溢出多为 UB）\n");
    printf("    • 优先级：~ > << >> > & > ^ > | ，但一律加括号（& 甚至低于 ==）\n");
    printf("    • << >> 都比 + - 低：1 << 2 + 3 是 32\n");
    printf("    • 位运算**没有短路**，也没有求值顺序保证（&& || 才有）\n");
    printf("    • 掩码常量写十六进制或 std::byte，别写十进制\n");

    return 0;
}
