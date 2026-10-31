// 4.9-1-sizeof-traps.cpp —— sizeof 的九个真相：编译期常量、不求值、退化、对齐
//
// 编译运行：
//   clang++ -std=c++17 -Wall -O2 4.9-1-sizeof-traps.cpp -o a && ./a

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// ---------- 对齐与填充 ----------
struct Packed_ish {
    char  c;    // 1
    int   i;    // 4 → 前面补 3
    char  d;    // 1 → 末尾补 3（保证数组里每个元素都对齐）
};
struct Reordered {
    int   i;    // 4
    char  c;    // 1
    char  d;    // 1 → 末尾补 2
};
struct Empty {};                 // sizeof = 1，不能是 0
struct WithVirtual { virtual ~WithVirtual() = default; };   // 多一个 vptr

int  g_arr[10] = {};
void takes_array(int a[10]) {    // 形参其实是 int*
    printf("    形参里 sizeof(a) = %zu（退化成指针，不是 40）\n", sizeof(a));
}

int main() {
    printf("=== 4.9-1 sizeof 的九个真相 ===\n\n");

    printf("[1] 它是**编译期**常量，不是运行时函数\n");
    int  fixed[sizeof(int) * 4];           // 可以当数组维度
    printf("    int fixed[sizeof(int)*4] 的元素数 = %zu\n", sizeof(fixed) / sizeof(fixed[0]));
    static_assert(sizeof(int) == 4, "编译期就能断言");
    printf("    static_assert(sizeof(int)==4) 通过\n\n");

    printf("[2] **不对操作数求值**——这是最容易踩的一条\n");
    int i = 5;
    size_t s = sizeof(i++);
    printf("    sizeof(i++) 之后 i = %d（++ 根本没执行）\n", i);
    printf("    sizeof(i++) = %zu\n", s);
    printf("    同理 sizeof(f()) 不会真的调用 f —— 只要类型够，值不看\n\n");

    printf("[3] 数组 vs 指针：退化是 sizeof 的头号陷阱\n");
    printf("    sizeof(g_arr)  = %zu（定义处，拿到整个数组字节数）\n", sizeof(g_arr));
    printf("    sizeof(g_arr)/sizeof(g_arr[0]) = %zu 个元素 ← 惯用法\n",
           sizeof(g_arr) / sizeof(g_arr[0]));
    takes_array(g_arr);
    printf("    教训：数组一传参就退化成指针，函数内部**拿不到长度**，必须额外传 size\n\n");

    printf("[4] 字符串字面量含结尾的 \\0\n");
    printf("    sizeof(\"abc\") = %zu，strlen(\"abc\") = %zu（差 1 就是 \\0）\n",
           sizeof("abc"), strlen("abc"));
    const char* p = "abc";
    printf("    但赋给指针后 sizeof(p) = %zu ← 退化\n\n", sizeof(p));

    printf("[5] 对齐与填充：成员顺序会改变结构体大小\n");
    printf("    struct { char; int; char; }  sizeof = %zu\n", sizeof(Packed_ish));
    printf("    struct { int; char; char; }  sizeof = %zu  ← 换个顺序省 %zu 字节\n",
           sizeof(Reordered), sizeof(Packed_ish) - sizeof(Reordered));
    printf("    规则：成员按自身对齐值对齐，结构体整体对齐 = 最大成员对齐值\n\n");

    printf("[6] 空类是 1 字节，不是 0\n");
    printf("    sizeof(Empty) = %zu\n", sizeof(Empty));
    printf("    为什么不能是 0：否则两个不同对象的地址会相同，指针无法区分\n");
    Empty e1, e2;
    printf("    &e1 = %p, &e2 = %p（必须不同）\n", (void*)&e1, (void*)&e2);
    printf("    sizeof(WithVirtual) = %zu ← 有虚函数，多一个 vptr（8 字节）\n\n",
           sizeof(WithVirtual));

    printf("[7] sizeof 引用 = 被引用类型的大小（不是「引用本身」的大小）\n");
    int  x = 0;
    int& rx = x;
    double d = 0;
    double& rd = d;
    printf("    sizeof(rx) = %zu（= sizeof(int)）\n", sizeof(rx));
    printf("    sizeof(rd) = %zu（= sizeof(double)）\n", sizeof(rd));
    printf("    引用没有自己的大小概念，它就是别名\n\n");

    printf("[8] 标准库类型的大小：SSO 与三指针结构\n");
    printf("    sizeof(std::string)      = %zu（libc++：ptr + size + SSO 联合体）\n",
           sizeof(std::string));
    printf("    sizeof(std::vector<int>) = %zu（begin + end + capacity 三指针）\n",
           sizeof(std::vector<int>));
    std::string short_s = "abc";
    std::string long_s  = "0123456789012345678901234567890";   // 31 字符，超过 SSO
    printf("    短字符串 \"%s\" 的 c_str() 距对象首地址 %ld 字节（在对象内部 = SSO）\n",
           short_s.c_str(), (long)((const char*)short_s.c_str() - (const char*)&short_s));
    printf("    长字符串 的 c_str() 距对象首地址 %ld 字节（负数/很远 = 堆分配）\n\n",
           (long)((const char*)long_s.c_str() - (const char*)&long_s));

    printf("[9] 不能对什么用 sizeof\n");
    printf("    ❌ 函数类型：sizeof(void())  —— 编译错误\n");
    printf("    ❌ 不完整类型：sizeof(struct Undefined)\n");
    printf("    ❌ void：sizeof(void)（GCC 扩展允许为 1，标准不允许）\n");
    printf("    ✅ 位域只能配合对象：sizeof(BitFieldStruct)\n");
    printf("    sizeof 返回 size_t（无符号！）—— 与 int 比较前先想清楚符号问题：\n");
    printf("      sizeof(int) - 5 = %zu ← 无符号下溢，不是 -1\n", sizeof(int) - 5);
    return 0;
}

// ---------------------------------------------------------------------------
// 取消注释可复现的编译错误：
//   sizeof(void())        → error: invalid application of 'sizeof' to a function type
//   sizeof(struct Undef)  → error: invalid application of 'sizeof' to an incomplete type
//   sizeof(void)          → error: invalid application of 'sizeof' to 'void'
//                           （-pedantic 下；clang 默认也报错）
