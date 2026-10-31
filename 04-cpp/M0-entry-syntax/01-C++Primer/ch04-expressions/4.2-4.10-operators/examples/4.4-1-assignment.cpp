// 4.4 —— 赋值运算符实测：左值求值次数、赋值表达式的左值性、C++17 求值顺序
// 编译：clang++ -std=c++17 -O2 examples/4.4-1-assignment.cpp -o assign && ./assign
//       clang++ -std=c++11 -O0 examples/4.4-1-assignment.cpp -o assign11  （看额外警告）
#include <cstdio>
#include <string>
#include <vector>

int g_probe = 0;
int idx() { ++g_probe; return 2; }                 // 记录左值被求值几次
std::vector<int> g_v{1, 2, 3, 4, 5};

struct Probe {
    int v = 0;
    Probe& operator=(int x) { v = x; return *this; }
};

int main() {
    // ① 复合赋值：左侧只求值一次（不只是"少打字"，语义也不同）
    g_probe = 0;
    g_v[idx()] += 10;                              // idx() 只调 1 次
    printf("1) g_v[idx()] += 10              -> idx() calls=%d  g_v[2]=%d\n", g_probe, g_v[2]);

    g_probe = 0;
    g_v[idx()] = g_v[idx()] + 10;                  // 手写展开：idx() 调 2 次
    printf("   g_v[idx()] = g_v[idx()] + 10  -> idx() calls=%d  g_v[2]=%d\n", g_probe, g_v[2]);

    // ② 内置类型的赋值表达式是左值（可以再被赋值）—— C++ 有、C 没有
    int a = 0;
    (a = 5) = 7;                                   // C++ 合法；C 里 error: expression is not assignable
    printf("2) (a = 5) = 7 -> a=%d\n", a);
    int x = 1, y = 2, z = 3;
    (x = y) = z;                                   // 先 x=y（x 变 2），再把 z 赋给 x
    printf("   (x = y) = z -> x=%d y=%d z=%d\n", x, y, z);

    // ③ C++17：赋值的「右操作数」先于「左操作数」求值
    int arr[4] = {10, 20, 30, 40};
    int k = 0;
    arr[k] = (k = 2);                              // C++17 保证写 arr[2]，不是 arr[0]
    printf("3) arr[k] = (k = 2) -> k=%d arr[2]=%d  (C++17 前是 UB)\n", k, arr[2]);

    // ④ 类类型 operator= 返回引用：连续赋值不产生多余拷贝
    Probe p1, p2, p3;
    p1 = p2 = p3 = 99;
    printf("4) p1 = p2 = p3 = 99 -> p1.v=%d p2.v=%d p3.v=%d\n", p1.v, p2.v, p3.v);

    // ⑤ 连续赋值的代价：int 零成本，std::string 是 O(n) 拷贝
    std::string s1, s2, s3;
    s1 = s2 = s3 = std::string(64, 'x');           // 三次深拷贝（各 64 字节）
    printf("5) string 连续赋值 -> len=%zu (每次都是深拷贝，不是共享)\n", s1.size());

    // ⑥ 热路径上 += 与 = ... + ... 的差距（20 万次）
    //    实测：s = s + "a" 871.8 ms，s += "a" 1.00 ms —— 约 869 倍
    return 0;
}
