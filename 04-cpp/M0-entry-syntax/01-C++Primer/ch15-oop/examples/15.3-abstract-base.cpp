// 15.3 抽象基类与访问控制 —— 纯虚函数强制派生类实现接口
// 实测：clang++ -std=c++20 -Wall 15.3-abstract-base.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>
#include <memory>
#include <vector>
#include <cmath>

// 策略接口：只声明"能做什么"，不声明"怎么做"
class Strategy {
public:
    virtual ~Strategy() = default;          // 多态基类必须虚析构（见 13 章）
    virtual double signal(double mid) const = 0;   // 纯虚 -> Strategy 是抽象类
};

// 抽象类不能创建对象：
//   Strategy s;   // error: allocating an object of abstract class type

class MaCross : public Strategy {
public:
    double signal(double mid) const override { return mid > 100 ? 1.0 : -1.0; }
};

class MeanRevert : public Strategy {
public:
    explicit MeanRevert(double anchor) : anchor_(anchor) {}
    double signal(double mid) const override { return (anchor_ - mid) / anchor_; }
private:
    double anchor_;
};

// 函数只认接口，完全不知道具体策略是什么 —— 依赖倒置
static void dispatch(const Strategy& s, double mid) {
    double sig = s.signal(mid);
    printf("  mid=%.2f -> 信号 %+.2f %s\n", mid, sig,
           sig > 0.5 ? "买" : sig < -0.5 ? "卖" : "持有");
}

int main() {
    std::vector<std::unique_ptr<Strategy>> book;   // 15.5 详说为什么用指针容器
    book.push_back(std::make_unique<MaCross>());
    book.push_back(std::make_unique<MeanRevert>(100.0));

    for (double mid : {99.0, 101.0}) {
        printf("行情 mid = %.2f:\n", mid);
        for (const auto& s : book) dispatch(*s, mid);
    }

    // 价值拆解：
    // 1. 纯虚 = 编译器强制"实现接口才有资格实例化"（漏写 override 直接编译失败）
    // 2. dispatch 不 include 任何具体策略头文件 -> 加策略零改动
    // 3. 虚析构保证经基类指针 delete 时派生部分被正确析构
    printf("\n抽象基类 = 合同；派生类 = 签字画押。\n");
    return 0;
}
