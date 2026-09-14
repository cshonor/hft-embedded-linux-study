// 7.2 访问控制与封装 —— private 是编译期检查，运行期零开销
// 实测：clang++ -std=c++20 -Wall 7.2-access-control.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>
#include <cstring>

// class 与 struct 的唯一区别：默认访问级别（class=private, struct=public）
// 下面两个内存布局完全相同，sizeof 相同
class OrderByClass {
    long    qty_;      // class: 默认 private
    double  price_;
public:
    OrderByClass(long q, double p) : qty_(q), price_(p) {}
    double notional() const { return qty_ * price_; }
};

struct OrderByStruct {
    long    qty_;      // struct: 默认 public（仍可写 private:）
    double  price_;
    OrderByStruct(long q, double p) : qty_(q), price_(p) {}
    double notional() const { return qty_ * price_; }
};

int main() {
    OrderByClass a(100, 12.5);
    OrderByStruct b(100, 12.5);
    printf("sizeof(class 版)  = %zu 字节\n", sizeof(a));
    printf("sizeof(struct 版) = %zu 字节\n", sizeof(b));
    printf("两者内存布局完全相同: %s\n",
           memcmp(&a, &b, sizeof(a)) == 0 ? "是" : "否");

    printf("名义本金 = %.2f\n", a.notional());   // 只能走 public 接口

    // 直接访问私有成员？编译器直接拒绝（注释掉体验一下）：
    // long q = a.qty_;
    //   error: 'qty_' is a private member of 'OrderByClass'

    // 关键认知：private 检查只发生在编译期。
    // 生成的机器码里没有任何"访问检查"指令 —— 封装是免费的。
    // 对照：Python 的 _name 只是约定，Java 的反射能绕过；C++ 的 private
    // 是编译器级的硬约束（除非内存布局碰巧可 reinterpret，那是 UB）。
    printf("\n结论: 封装 = 编译期约束, 运行期零开销（HFT 关心后者）。\n");
    return 0;
}
