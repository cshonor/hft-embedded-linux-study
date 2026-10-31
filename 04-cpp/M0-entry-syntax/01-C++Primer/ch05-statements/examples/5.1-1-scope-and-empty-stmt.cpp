// 5.1–5.2 块作用域、生命周期、空语句、C++17 带初始化的 if / switch
//
// 编译：clang++ -std=c++17 -O2 5.1-1-scope-and-empty-stmt.cpp -o scope && ./scope
#include <cstdio>

// RAII 探针：看构造/析构发生在哪一步
struct Probe {
    const char* name;
    explicit Probe(const char* n) : name(n) { printf("    + 构造 %s\n", name); }
    ~Probe() { printf("    - 析构 %s\n", name); }
};

int get_value() { return 3; }

int main(int argc, char**) {
    printf("① for 循环体里的对象：每轮构造 + 析构\n");
    for (int i = 0; i < 2; ++i) {
        Probe p("loop-body");        // 每轮迭代都构造一次、结束时析构
        printf("    i=%d\n", i);
    }

    printf("② 块结束即析构（手动加 {} 缩短生命周期）\n");
    {
        Probe a("block-a");
        {
            Probe b("block-b");
        }                            // b 在这里析构
        printf("    （b 已销毁）\n");
    }

    printf("③ 离开作用域后内存是否复用由编译器决定，不可依赖\n");
    {
        int x = 1; printf("    块1 x 地址 %p\n", (void*)&x);
    }
    {
        int x = 2; printf("    块2 x 地址 %p\n", (void*)&x);   // 实测：相邻但不同
    }

    printf("④ 遮蔽（shadow）：内层同名变量盖住外层\n");
    int v = 100;
    {
        int v = 200;                 // 遮蔽外层 v（-Wshadow 会警告）
        printf("    内层 v=%d\n", v);
    }
    printf("    外层 v=%d\n", v);

    printf("⑤ C++17 带初始化的 if / switch：变量只在条件结构内可见\n");
    if (int n = get_value(); n > 0) {          // n 的作用域覆盖 if + else
        printf("    if 分支 n=%d\n", n);
    } else {
        printf("    else 分支 n=%d\n", n);
    }
    switch (int k = argc; k) {                 // k 的作用域覆盖整个 switch
        case 1: printf("    switch k=%d\n", k); break;
        default: printf("    switch default k=%d\n", k); break;
    }

    printf("⑥ 空语句坑（下面这行会触发 -Wempty-body）\n");
    int n = 5;
    int i = 0;
    while (i < n) { ++i; }                                   // 正确写法
    printf("    正常 while 结束 i=%d\n", i);

    int j = 0, guard = 0;
    while (j < n && ++guard < 50);   // ← 分号让循环体成空语句（guard 兜底防真死循环）
    { ++j; }
    printf("    空语句版：j=%d（循环体一次都没执行 j，下面 {} 只是普通块）\n", j);
    return 0;
}
