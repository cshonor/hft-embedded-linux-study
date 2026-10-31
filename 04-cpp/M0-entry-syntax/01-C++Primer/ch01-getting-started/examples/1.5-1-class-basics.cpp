// 1.5-1  类简介 —— item.isbn() 底层就是 isbn(&item)：汇编级证明、this 指针、sizeof
//
// 编译运行：clang++ -std=c++17 -O2 -Wall 1.5-1-class-basics.cpp -o cls && ./cls
// 汇编对照（本例的核心）：
//   clang++ -std=c++17 -O1 -S 1.5-1-class-basics.cpp -o cls.s
//   在 cls.s 里比较 __Z8free_sumPK3Box（C 风格）与 __Z11call_memberPK6BoxCxx
//   （C++ 风格，成员函数已被内联进去）的函数体 —— 逐条指令相同
//
// 核心结论（clang 23.1.0 / arm64 实测）：
//   ① 成员函数 = 普通函数 + 隐式的 this 指针参数
//      -O1 下 free_sum(const Box*) 与 Box::sum() const 的函数体【逐条指令相同】：
//          ldp  x8, x9, [x0]
//          ldr  x10, [x0, #16]
//          add  x8, x9, x8
//          add  x0, x8, x10
//          ret
//      差别只在符号名：_Z8free_sumPK3Box  vs  _ZNK3Box3sumEv
//      （用 c++filt 还原：free_sum(Box const*)  vs  Box::sum() const）
//   ② this == &obj（实测地址完全相同）
//   ③ 非虚成员函数【不占对象空间】：加 3 个方法后 sizeof 不变
//   ④ 一旦引入虚函数，对象多出一个 vptr（arm64 上 8 字节）
//   ⑤ 空类 sizeof == 1（保证两个不同对象地址不同）

#include <iostream>

struct Box { long a; long b; long c; };

// C 风格：显式传指针
long free_sum(const Box* p) { return p->a + p->b + p->c; }

// C++ 风格：成员函数（this 就是隐式的 p）
struct BoxCxx {
    long a, b, c;
    long sum() const { return a + b + c; }
};
long call_member(const BoxCxx* p) { return p->sum(); }   // 供汇编对照

// ---- ③ sizeof 对照 ----
struct Empty { };                                   // 空类
struct DataOnly { double px; int qty; };            // 只有数据
struct WithMethods {                                // 数据 + 一堆方法
    double px; int qty;
    double price() const { return px; }
    int    size()  const { return qty; }
    void   set(double p, int q) { px = p; qty = q; }
};
struct WithVirtual {                                // 有虚函数 → 多一个 vptr
    double px; int qty;
    virtual ~WithVirtual() { }
};

// ---- ② this 指针 ----
struct Tick {
    double px; int qty;
    const Tick* me() const { return this; }
};

int main() {
    Box    b{1, 2, 3};
    BoxCxx c{1, 2, 3};
    std::cout << "free_sum(&b) = " << free_sum(&b) << "\n";
    std::cout << "c.sum()      = " << c.sum()      << "\n";
    std::cout << "两者结果相同，且 -O1 汇编逐条一致（见文件头说明）\n\n";

    Tick t{10.5, 100};
    std::cout << "  &t     = " << &t << "\n"
              << "  t.me() = " << t.me() << "\n"
              << "  this == &t ? " << (&t == t.me() ? "是" : "否") << "\n\n";

    std::cout << "sizeof(Empty)       = " << sizeof(Empty) << "   （空类不能是 0）\n";
    std::cout << "sizeof(DataOnly)    = " << sizeof(DataOnly) << "\n";
    std::cout << "sizeof(WithMethods) = " << sizeof(WithMethods) << "   （加了 3 个方法，大小不变）\n";
    std::cout << "sizeof(WithVirtual) = " << sizeof(WithVirtual) << "   （多一个 vptr）\n";

    return 0;
}
