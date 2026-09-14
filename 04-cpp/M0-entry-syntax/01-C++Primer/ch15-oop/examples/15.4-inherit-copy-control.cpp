// 15.4 继承中的拷贝控制与类作用域 —— 构造/析构顺序、名字隐藏
// 实测：clang++ -std=c++20 -Wall 15.4-inherit-copy-control.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>

class Base {
public:
    Base()            { printf("  [构造] Base\n"); }
    Base(const Base&) { printf("  [拷贝] Base\n"); }
    ~Base()           { printf("  [析构] Base\n"); }

    void send()        { printf("  Base::send()\n"); }
    void send(int qos) { printf("  Base::send(qos=%d)\n", qos); }
};

class Derived : public Base {
public:
    Derived() { printf("  [构造] Derived（Base 部分先构造好）\n"); }
    ~Derived() { printf("  [析构] Derived\n"); }

    // 名字隐藏：派生类里任何 send 都会遮蔽基类的全部 send 重载
    void send(const char* topic) {
        printf("  Derived::send(topic=%s)\n", topic);
    }
};

int main() {
    printf("构造一个 Derived:\n");
    {
        Derived d;      // Base 先生，Derived 后生；析构严格反序
    }
    printf("出了作用域，看析构顺序（Derived 先走，Base 后走）\n\n");

    printf("名字隐藏实验:\n");
    Derived d;
    printf("调用 d.send(\"x\"): \n");
    d.send("market-data");      // 派生版本

    // 基类的两个 send 被整个遮蔽了（解注释体验编译错误）：
    // d.send();      // error: no matching member function（找不到无参版）
    // d.send(1);     // error: 不能把 int 转成 const char*
    printf("  d.send() / d.send(1) 无法编译 —— int 版被 topic 版遮蔽\n");

    // 修法一：基类指针/引用调用
    Base& b = d;
    b.send();
    // 修法二：using 声明把基类重载引进来
    // 在 Derived 里加: using Base::send;
    printf("\n规则: 查名按作用域找，找到即停 —— 基类作用域排在派生类之后。\n");
    return 0;
}
