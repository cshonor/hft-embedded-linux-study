// 16.3 模板实参推断 —— 编译器从实参反推 T；数组大小也能被推出来
// 实测：clang++ -std=c++20 -Wall 16.3-deduction.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>

// 经典推断案例：T(&arr)[N] —— 对"数组引用"做模板，
// N 从实参数组上直接推出来。运行期写的 sizeof(arr)/sizeof(arr[0]) 没有这个能力
template <typename T, std::size_t N>
constexpr std::size_t count_of(const T (&arr)[N]) {
    return N;
}

// 推断规则展示：模板参数按值接收时，数组/函数会退化成指针（decay）
template <typename T> void by_value(T) {
    printf("  按值:   %s\n", __PRETTY_FUNCTION__);
}
// 按引用接收时保留原貌
template <typename T> void by_ref(const T&) {
    printf("  按引用: %s\n", __PRETTY_FUNCTION__);
}

int main() {
    long ticks[7];
    double px[64];
    const char* msg = "hello";

    printf("数组大小推断（不用手写 sizeof 技巧）:\n");
    printf("  long[7]    -> %zu\n", count_of(ticks));
    printf("  double[64] -> %zu\n", count_of(px));
    // count_of(msg) 会推成 const char*&，没有 N -> 编译错误，正好挡住误用
    // count_of(msg);   // error: no matching function

    printf("\n同一场数组，两种传法得到不同 T:\n");
    by_value(ticks);    // T 退化为 long*        —— 丢失了"是数组"的信息
    by_ref(ticks);      // T = long[7]          —— 原貌保留
    // __PRETTY_FUNCTION__ 输出会展示 T 的真实身份，这就是"推断发生了什么"的证据

    // 显式指定：编译器推不动或推错时，人来拍板
    // by_value<double>(3);  // T = double，3 被转换（不再推成 int）
    printf("\n推断 = 编译器的模式匹配；按值会 decay，按引用保原样。\n");
    return 0;
}
