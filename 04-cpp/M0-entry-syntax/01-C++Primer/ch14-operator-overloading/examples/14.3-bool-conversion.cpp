// 14.3 类型转换运算符 —— explicit operator bool：能在 if 里用，但不能乱转
// 实测：clang++ -std=c++20 -Wall 14.3-bool-conversion.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>

// 热路径上的可选结果：要么有价格，要么没有
// （真实项目里等价物是 std::optional<Price>）
struct Quote {
    explicit Quote(long t) : has_(t > 0), ticks_(t) {}

    // 不加 explicit 的灾难（想象一下）：
    //   Quote q(0);  int x = q + 1;        // bool->int 隐式转换，x=1 ??
    //   if (q == 1) ...                    // 引用比较变成数值比较 ??
    // 加了 explicit 后只有"上下文明确要求 bool"的场合才转换：
    explicit operator bool() const { return has_; }

    long ticks() const { return ticks_; }
private:
    bool has_;
    long ticks_;
};

int main() {
    Quote valid(12345800);
    Quote empty(0);

    // 合法：if/while/! /&&/|| 这些是"上下文转换"，explicit 不拦
    if (valid) printf("valid: 有行情 %ld ticks\n", valid.ticks());
    if (!empty) printf("empty: 无行情\n");

    // 非法：explicit 挡住了意外转换（解注释体验编译错误）：
    // long x = valid;            // error: no viable conversion
    // long y = valid + 1;        // error
    // printf("%ld", valid);      // error —— printf 上下文不"要求 bool"

    // 需要真 bool 时必须显式说：
    bool b = static_cast<bool>(valid);
    printf("显式转换后 b = %d\n", b);

    // 原则：转换运算符一律 explicit（std::optional、智能指针全是这么做的），
    // 让"能当条件用"和"能当别的类型用"彻底分开。
    printf("\nexplicit operator bool = 只开 if() 这一扇门。\n");
    return 0;
}
