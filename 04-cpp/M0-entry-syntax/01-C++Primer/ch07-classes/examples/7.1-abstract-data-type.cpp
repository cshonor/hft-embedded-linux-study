// 7.1 定义抽象数据类型 —— this 指针、const 成员函数、return *this 链式调用
// 实测：clang++ -std=c++20 -Wall 7.1-abstract-data-type.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>

// 订单价格（HFT 里价格用整数 tick 表示，避免浮点误差）
class Price {
public:
    // 构造函数：初始化 tick 数
    Price(long ticks) : ticks_(ticks) {}

    // const 成员函数：承诺不修改对象（this 的类型是 const Price*）
    long ticks() const { return ticks_; }
    double price() const { return ticks_ / 10000.0; }   // 1 tick = 0.0001 元

    // 非常量成员函数：可以修改对象。注意 this 就是当前对象的地址
    Price& add(long ticks) {
        printf("  this = %p（正在修改这个地址上的对象）\n", (void*)this);
        ticks_ += ticks;
        return *this;   // 解引用 this 再返回 -> 支持链式调用
    }

    // 链式调用：每个操作都返回 *this，可以一路 . 下去
    // 若把上面 add 写成 const，ticks_ += 会编译报错：
    //   error: cannot assign to variable 'ticks_' with const-qualified type

private:
    long ticks_;    // 私有成员：外部只能通过上面的接口访问
};

int main() {
    Price p(12345678);                       // 1234.5678 元
    printf("初始: %.4f 元 (%ld ticks)\n", p.price(), p.ticks());

    printf("链式调用 add():\n");
    p.add(100).add(200).add(300);            // 三个 add 作用在同一个对象上
    printf("最终: %.4f 元 (%ld ticks)\n", p.price(), p.ticks());

    // 关键验证：三次 add 的 this 是不是同一个地址？
    printf("\n三次 add 各打印一次 this —— 回头看上面，三个地址一样。\n");
    printf("这就是「成员函数知道自己在操作谁」的机制：编译器把 p.add(100)\n");
    printf("改写成 add(&p, 100) —— this 就是那个隐式传进来的 &p。\n");
    return 0;
}
