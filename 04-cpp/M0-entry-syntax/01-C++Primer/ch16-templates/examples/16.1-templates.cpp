// 16.1 定义模板 —— 一份代码，编译器按类型生成多份；每个实例化都是独立世界
// 实测：clang++ -std=c++20 -Wall 16.1-templates.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>

template <typename T>
T max_of(const T& a, const T& b) {
    // __PRETTY_FUNCTION__ 会展示"这份实例"的全名 —— 证明编译器生成了几份代码
    printf("  实例化自: %s\n", __PRETTY_FUNCTION__);
    return a > b ? a : b;
}

template <typename T>
class Counter {
public:
    void hit() { ++n_; }
    long n() const { return n_; }
private:
    long n_ = 0;
};

int main() {
    printf("调用 max_of(3, 7):\n");        // T = int   -> 生成 int 版
    printf("  结果 = %ld\n", (long)max_of(3, 7));
    printf("调用 max_of(1.5, 2.5):\n");    // T = double -> 生成 double 版
    printf("  结果 = %f\n", max_of(1.5, 2.5));

    // T 推断冲突：max_of(3, 1.5) 编译失败 —— int 和 double 要一个 T 签不下来
    // max_of(3, 1.5);   // error: deduced conflicting types for 'T'

    // 关键演示：Counter<int> 和 Counter<double> 是两个完全独立的类，
    // 静态数据互不可见、sizeof 各算各的 —— 模板不是"一份代码"，
    // 是"一个代码工厂"，每个类型进厂都铸出一份新代码
    Counter<int>   ci;
    Counter<double> cd;
    Counter<int>   ci2;
    ci.hit(); ci.hit();
    cd.hit();
    ci2.hit();
    printf("\nCounter<int>(1号)   命中 %ld 次\n", ci.n());
    printf("Counter<int>(2号)   命中 %ld 次（同一个实例化，各对象各一份 n_）\n", ci2.n());
    printf("Counter<double>     命中 %ld 次（另一个实例化，与 int 版毫无瓜葛）\n", cd.n());
    return 0;
}
