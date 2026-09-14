// 16.4 高级特性 —— 特化与可变参模板：定制 + 一口吞下任意多个参数
// 实测：clang++ -std=c++20 -Wall 16.4-advanced.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>
#include <string>

// ---------- 全特化：给某个具体类型换一份专门实现 ----------
template <typename T>
struct TypeName {
    static const char* get() { return "未知类型（通用版）"; }
};
template <>                       // 全特化：T = const char* 时走这份
struct TypeName<const char*> {
    static const char* get() { return "C 字符串（特化版）"; }
};

// ---------- 可变参模板：参数包 ----------
// 递归展开：每次剥掉一个参数，直到空包触发终止函数
inline void sum_all(long& acc) {}                     // 终止：空包

template <typename... Rest>                            // Rest 是参数包
void sum_all(long& acc, long first, Rest... rest) {
    acc += first;
    sum_all(acc, rest...);                             // 递归剥一层
}

template <typename... Args>
long latency_us(Args... stage_costs) {                 // 各阶段耗时打包进来
    long total = 0;
    sum_all(total, stage_costs...);
    return total;
}

int main() {
    // 特化：同一次调用，类型不同走不同实现
    printf("TypeName<int>::get()        = %s\n", TypeName<int>::get());
    printf("TypeName<const char*>::get() = %s\n",
           TypeName<const char*>::get());
    // 注意 42 和 "42" 是不同类型 -> 特化机制在这里自动分岔

    // 可变参：一次算出整条链路的延迟（解析->决策->发送）
    long l1 = latency_us(3, 11, 2);            // 3 个阶段
    long l2 = latency_us(3, 11, 2, 7, 1, 4);   // 6 个阶段，同一个函数
    printf("\n3 阶段总延迟 = %ld us\n", l1);
    printf("6 阶段总延迟 = %ld us\n", l2);

    // 模板世界的三板斧，这里见到第二、第三把：
    //   特化     = "对个别类型另写一份"（STL 里 hash/vector<bool> 都靠它）
    //   参数包   = "参数个数也变成模板参数"（make_unique/.emplace_back 的地基）
    printf("\n特化管类型维度的定制，参数包管个数维度的定制。\n");
    return 0;
}
