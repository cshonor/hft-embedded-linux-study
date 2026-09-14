// 7.6 类的静态成员 —— 全类共享一份，不占对象空间
// 实测：clang++ -std=c++20 -Wall 7.6-static-members.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>

class Order {
public:
    explicit Order(long id) : id_(id) { ++live_; }        // 出生
    ~Order()                     { --live_; }              // 死亡
    long id() const { return id_; }

    // 静态成员函数：没有 this，不能访问非静态成员
    static long live_count() { return live_; }
    //    static long bad() { return id_; }   // error: 无 this，摸不到 id_

private:
    long id_;
    static long live_;      // 声明：所有 Order 共享这一份
};

long Order::live_ = 0;      // 定义：类外且只此一次（必须有，否则链接错误）

int main() {
    printf("初始存活订单 = %ld\n", Order::live_count());

    {
        Order a(1), b(2), c(3);
        printf("构造 3 个后 = %ld\n", Order::live_count());
        printf("sizeof(Order) = %zu 字节 —— 只有 id_，不含静态成员\n",
               sizeof(Order));
    }   // a/b/c 在这里析构
    printf("出了作用域 = %ld\n", Order::live_count());

    // 两种等价的调用方式：
    Order x(7);
    printf("通过对象调用 = %ld，通过类名调用 = %ld\n",
           x.live_count(), Order::live_count());

    // HFT 视角：live_ 这类计数器就是对象池/订单管理器的核心状态。
    // 静态成员 = 链接器层面的全局变量 + 类作用域的访问控制，
    // 存放在 .data/.bss，不进任何对象 —— 所以 sizeof 不变。
    return 0;
}
