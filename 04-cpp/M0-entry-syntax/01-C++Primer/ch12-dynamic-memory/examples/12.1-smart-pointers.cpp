// 12.1 智能指针 —— 可跑例子
// 看 unique_ptr 的独占、shared_ptr 的引用计数、weak_ptr 怎么打破循环引用
//
// 编译运行：
//   clang++ -std=c++20 12.1-smart-pointers.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>
#include <memory>

struct Node {
    int id;
    Node(int i) : id(i) { std::printf("    [构造] Node(%d)\n", id); }
    ~Node() { std::printf("    [析构] Node(%d)\n", id); }
};

struct Bad {                                  // 循环引用版
    std::shared_ptr<Bad> peer;
    int id;
    Bad(int i) : id(i) { std::printf("    [构造] Bad(%d)\n", id); }
    ~Bad() { std::printf("    [析构] Bad(%d)\n", id); }
};

struct Good {                                 // 用 weak_ptr 打破
    std::weak_ptr<Good> peer;
    int id;
    Good(int i) : id(i) { std::printf("    [构造] Good(%d)\n", id); }
    ~Good() { std::printf("    [析构] Good(%d)\n", id); }
};

int main() {
    std::printf("--- 1. unique_ptr：独占所有权，不能复制 ---\n");
    {
        std::unique_ptr<Node> a(new Node(1));
        std::printf("    a 持有对象? %s\n", a ? "是" : "否");
        std::unique_ptr<Node> b = std::move(a);      // 只能转移，不能复制
        std::printf("    转移后：a 持有? %s    b 持有? %s\n",
                    a ? "是" : "否", b ? "是" : "否");
    }
    std::printf("    -> 离开作用域自动析构\n\n");

    std::printf("--- 2. shared_ptr：引用计数 ---\n");
    {
        std::shared_ptr<Node> p1(new Node(2));
        std::printf("    p1 计数 = %ld\n", p1.use_count());

        std::shared_ptr<Node> p2 = p1;
        std::printf("    p2 = p1 后计数 = %ld\n", p1.use_count());

        std::shared_ptr<Node> p3 = p2;
        std::printf("    p3 = p2 后计数 = %ld\n", p1.use_count());

        p3.reset();
        std::printf("    p3.reset() 后计数 = %ld\n", p1.use_count());
    }
    std::printf("    -> 计数归零才真正析构\n\n");

    std::printf("--- 3. 循环引用：两个 shared_ptr 互相持有 ---\n");
    {
        auto x = std::make_shared<Bad>(10);
        auto y = std::make_shared<Bad>(11);
        x->peer = y;
        y->peer = x;
        std::printf("    离开作用域前：x 计数=%ld  y 计数=%ld\n",
                    x.use_count(), y.use_count());
    }
    std::printf("    ^ 上面没有 [析构] 输出 —— 计数永远归不了零，内存泄漏\n\n");

    std::printf("--- 4. 用 weak_ptr 打破循环 ---\n");
    {
        auto x = std::make_shared<Good>(20);
        auto y = std::make_shared<Good>(21);
        x->peer = y;
        y->peer = x;
        std::printf("    离开作用域前：x 计数=%ld  y 计数=%ld\n",
                    x.use_count(), y.use_count());
    }
    std::printf("    ^ 这次有 [析构] —— weak_ptr 不增加计数，不阻止释放\n");
    return 0;
}
