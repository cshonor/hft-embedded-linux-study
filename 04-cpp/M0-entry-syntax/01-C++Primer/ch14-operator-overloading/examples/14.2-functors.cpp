// 14.2 函数调用运算符与函数对象 —— 带状态的比较器，排序时数出比较次数
// 实测：clang++ -std=c++20 -Wall 14.2-functors.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>
#include <algorithm>
#include <functional>
#include <vector>

// 函数对象 = 重载了 operator() 的类。
// 与普通函数的区别：它是对象 -> 可以带状态（这里是比较计数器）
struct PriceLess {
    mutable long compares = 0;      // const 场景下也要累加
    // std::sort 要求比较器是 const 可调用的
    bool operator()(const long& a, const long& b) const {
        ++compares;                 // mutable 成员，const 函数里可写
        return a < b;
    }
};

int main() {
    std::vector<long> prices{12345800, 12345100, 12345900, 12345300,
                             12345700, 12345200, 12345600};

    PriceLess cmp;                          // 对象，不是函数
    // 注意 std::sort 的比较器参数是"按值"的 —— 直接传 cmp 会把计数器
    // 拷贝进 sort 内部，结束后原对象的 compares 还是 0。
    // std::ref 让 sort 拷贝的是"引用包装"，计数仍然落在 cmp 里。
    std::sort(prices.begin(), prices.end(), std::ref(cmp));
    // sort 内部拿着 cmp 的引用反复调用 cmp(a,b)，
    // 计数器在对象里活着 —— 函数指针做不到这一点

    printf("排序结果: ");
    for (auto p : prices) printf("%ld ", p);
    printf("\n");

    // lambda 本质上就是编译器给你生成的一个函数对象类：
    long count = 0;
    std::sort(prices.begin(), prices.end(),
              [&count](const long& a, const long& b) {
                  ++count;                  // 捕获的就是"状态"
                  return a > b;             // 反向排
              });
    printf("lambda 反向排序，比较 %ld 次\n", count);
    printf("升序那次比较了 %ld 次（可对照 14 个元素的 sort 理论值）\n",
           cmp.compares);

    // HFT 关联：策略回调、事件分发里大量用可调用对象，
    // 带状态 functor 比全局变量干净 —— 状态封装在对象里，可测试、可复制。
    return 0;
}
