// 12.2 直接内存管理 —— 可跑例子（二）
// 重点：delete 和 free 到底差在哪、new[] 的头部开销有多大
//
// 编译运行（注意两个标准下输出不同，这正是要看的点）：
//   clang++ -std=c++17 -O0 12.2-delete-vs-free.cpp -o /tmp/demo17 && /tmp/demo17
//   clang++ -std=c++11 -O0 12.2-delete-vs-free.cpp -o /tmp/demo11 && /tmp/demo11

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <new>

// ---------------------------------------------------------------
// 替换全局 operator new / delete，把编译器藏起来的动作全部打印出来
// ---------------------------------------------------------------
static std::size_t g_req  = 0;   // operator new[] 实际申请了多少字节
static void       *g_base = 0;   // operator new[] 拿到的起始地址

void *operator new(std::size_t n) {
    void *p = std::malloc(n);                 // ← 默认实现就是 malloc
    std::printf("    [op new]  请求 %zu 字节 -> malloc -> %p\n", n, p);
    return p;
}
void operator delete(void *p) noexcept {
    // 无 size 版本：C++11 走这条
    std::printf("    [op del]  %p  <- 无 size 版本 -> free\n", p);
    std::free(p);                             // ← 默认实现就是 free
}
void operator delete(void *p, std::size_t n) noexcept {
    // sized delete：C++14 起优先走这条
    std::printf("    [op del]  %p size=%zu  <- sized delete -> free\n", p, n);
    std::free(p);
}
void *operator new[](std::size_t n) {
    g_req = n;
    g_base = std::malloc(n);
    return g_base;
}
void operator delete[](void *p) noexcept { std::free(p); }

// ---------------------------------------------------------------
struct Tracked {
    int id;
    Tracked()      : id(-1) { std::printf("    [构造] Tracked(默认)\n"); }
    Tracked(int i) : id(i)  { std::printf("    [构造] Tracked(%d)\n", id); }
    ~Tracked()              { std::printf("    [析构] Tracked(%d)\n", id); }
};

int main() {
    std::printf("sizeof(Tracked) = %zu\n\n", sizeof(Tracked));

    // ===== 实验 1：delete 就是「析构 + free」 =====
    std::printf("========== 实验 1：new / delete 的完整链路 ==========\n");
    Tracked *a = new Tracked(1);
    delete a;
    std::printf("看上面 op del 走的是哪个版本 —— 编译器在 delete 点静态知道大小，\n");
    std::printf("C++14 起会优先把它传给 sized delete。\n\n");

    // ===== 实验 2：delete nullptr 是安全的 =====
    std::printf("========== 实验 2：delete nullptr ==========\n");
    Tracked *np = nullptr;
    delete np;              // 不崩，标准保证：什么也不做
    std::free(nullptr);     // free(NULL) 同样什么也不做
    std::printf("    程序继续往下跑 —— 两者都是 no-op，不需要写 if (p) 保护\n\n");

    // ===== 实验 3：new[] 的头部开销 =====
    std::printf("========== 实验 3：new[] 的头部(cookie) ==========\n");

    Tracked *arr = new Tracked[3];
    std::printf("    sizeof(Tracked)*3 = %zu 字节，但 operator new[] 申请了 %zu 字节\n",
                sizeof(Tracked) * 3, g_req);
    std::printf("    用户拿到的指针比起点往后挪了 %zu 字节（挪过去的就是头部）\n",
                (std::size_t)((char *)arr - (char *)g_base));
    delete[] arr;

    int *ints = new int[3];
    std::printf("    int[3]：sizeof*3 = %zu 字节，申请了 %zu 字节，偏移 %zu 字节\n",
                sizeof(int) * 3, g_req, (std::size_t)((char *)ints - (char *)g_base));
    delete[] ints;
    std::printf("    差别在于：int 的析构是平凡的，delete[] 不必逐个析构，\n");
    std::printf("    所以用不着记元素个数 —— 连头部都省了。\n\n");

    // ===== 实验 4：malloc 和 new 拿到的是同一片区域 =====
    std::printf("========== 实验 4：malloc 与 new 的地址 ==========\n");
    void *m = std::malloc(8);
    Tracked *t = new Tracked(2);
    std::printf("    malloc(8)       -> %p\n", m);
    std::printf("    new Tracked(2)  -> %p\n", (void *)t);
    std::printf("    两者相差 %td 字节 —— 同一个堆 arena，\n", (char *)t - (char *)m);
    std::printf("    因为 operator new 底层调的就是 malloc。\n");
    delete t;
    std::free(m);

    std::printf("\n要点:\n");
    std::printf("  new  T     = operator new(拿内存) + 构造   其中 operator new 默认 = malloc\n");
    std::printf("  delete p   = 析构 + operator delete        其中 operator delete 默认 = free\n");
    std::printf("  free(p)    = 只归还内存，不析构  <- 这就是它和 delete 唯一的区别\n");
    std::printf("  -> 混用不是「立刻崩」，而是「少做一半」：new+free 时析构函数根本不会跑。\n");
    return 0;
}
