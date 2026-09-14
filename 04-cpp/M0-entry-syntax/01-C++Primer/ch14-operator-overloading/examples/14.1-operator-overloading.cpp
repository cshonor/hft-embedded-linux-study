// 14.1 运算符重载 —— 给价格类型定义 + == << ，operator<< 必须是非成员
// 实测：clang++ -std=c++20 -Wall 14.1-operator-overloading.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>
#include <iostream>

// HFT 惯例：价格不用 double，用整数 tick（1 tick = 0.0001 元）
// 12.3456 元 = 123456 ticks —— 定点运算，没有浮点累积误差
class Price {
public:
    explicit Price(long ticks) : ticks_(ticks) {}

    // 复合赋值：成员函数（修改自身，this 隐式传入）
    Price& operator+=(const Price& rhs) {
        ticks_ += rhs.ticks_;
        return *this;
    }
    long ticks() const { return ticks_; }

private:
    long ticks_;
};

// 对称二元 + ：非成员函数（实现为 lhs 拷贝 += rhs）
inline Price operator+(Price lhs, const Price& rhs) {
    lhs += rhs;           // 复用 +=，经典惯用法
    return lhs;
}

inline bool operator==(const Price& a, const Price& b) {
    // 非成员也能写：只要有公开接口。a.ticks() 用的就是 7.1 的 const 接口
    return a.ticks() == b.ticks();
}

// operator<< 的左操作数是 ostream，不可能是 Price 的 this
// => 只能写成非成员（friend 与否取决于是否要摸私有成员）
inline std::ostream& operator<<(std::ostream& os, const Price& p) {
    return os << p.ticks() / 10000 << "."
              << (p.ticks() % 10000 < 1000 ? "0" : "")
              << p.ticks() % 10000;
}

int main() {
    Price bid(12345600);            // 1234.5600
    Price ask(12345800);            // 1234.5800

    Price mid = (bid + ask);        // operator+ -> operator+=
    // 注意 (bid+ask) 会隐式除 2 吗？不会 —— 这里演示的是运算符本身
    printf("bid + ask = %ld ticks\n", mid.ticks());

    std::cout << "bid = " << bid << ", ask = " << ask << "\n";
    std::cout << "bid == ask ? " << (bid == ask ? "true" : "false") << "\n";

    // 连续 << 能成立，是因为每次 operator<< 都返回 os 引用：
    // ((cout << "a") << "b") << "c"
    std::cout << "链式 << 靠的是返回流引用" << "\n";
    return 0;
}
