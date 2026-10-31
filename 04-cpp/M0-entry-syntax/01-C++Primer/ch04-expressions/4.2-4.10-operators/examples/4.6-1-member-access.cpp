// 4.6-1  成员访问运算符：. vs ->、优先级陷阱、operator-> 递归展开、->* 成员指针
//
// 编译：clang++ -std=c++17 -O2 -Wall 4.6-1-member-access.cpp -o mem && ./mem

#include <iostream>
#include <memory>
#include <vector>

struct Order { int qty{10}; double price{3.5}; };

struct Wrapper {                       // 单层代理：operator-> 返回裸指针
    Order o;
    Order* operator->() { return &o; }
};
struct Wrap2 {                         // 双层代理：operator-> 返回类对象
    Wrapper w;
    Wrapper operator->() { return w; }
};

int main() {
    Order* p = new Order{};

    std::cout << "=== ① 优先级陷阱：*p.member ===\n";
    std::cout << "  (*p).price = " << (*p).price << "    ✅ 括号改变优先级\n";
    std::cout << "  p->price   = " << p->price << "    ✅ 箭头\n";
    std::cout << "  *p.price   → 编译错误：'.' 优先级高于 '*'\n"
              << "                 解析成 *(p.price)，而 p 是指针没有成员\n";
    // std::cout << *p.price;   // ← 取消注释即编译失败

    std::cout << "\n=== ② 引用用 '.'，不是 '->' ===\n";
    Order& r = *p;
    std::cout << "  r.qty  = " << r.qty << "    ✅\n";
    std::cout << "  r->qty → 编译错误（引用在语法上是对象）\n";
    // std::cout << r->qty;     // ← 取消注释即编译失败
    std::cout << "  sizeof(Order*) = " << sizeof(Order*)
              << "   sizeof(r) = " << sizeof(r)
              << "   ← 引用的 sizeof 就是被引用对象的大小\n";

    std::cout << "\n=== ③ operator-> 会递归展开 ===\n";
    Wrapper w;
    std::cout << "  单层 Wrapper : w->qty  = " << w->qty << "\n";
    Wrap2 w2;
    std::cout << "  双层 Wrap2   : w2->qty = " << w2->qty
              << "   ← 返回类对象时编译器继续展开\n";
    std::cout << "  展开式：w2.operator->().operator->()->qty\n";

    std::cout << "\n=== ④ 智能指针：-> 能用，[] 不行 ===\n";
    auto up = std::make_unique<std::vector<int>>(std::initializer_list<int>{1, 2, 3});
    std::cout << "  up->size()  = " << up->size() << "    ✅\n";
    std::cout << "  (*up)[0]    = " << (*up)[0] << "    ✅ 先解引用再下标\n";
    std::cout << "  (*up).at(1) = " << (*up).at(1) << "    ✅\n";
    std::cout << "  up.get()    = " << static_cast<const void*>(up.get()) << "\n";
    std::cout << "  up[0]       → 编译错误：unique_ptr 没有 operator[]\n";
    std::cout << "  （注意：unique_ptr<T[]> 数组特化【有】operator[]）\n";

    std::cout << "\n=== ⑤ ->* 与 .* ：指向成员的指针（C++ 独有）===\n";
    auto pm = &Order::price;
    std::cout << "  p->*pm  = " << p->*pm << "    (等价 p->price)\n";
    Order o2{20, 9.9};
    std::cout << "  o2.*pm  = " << o2.*pm << "    (对象用 .* )\n";
    std::cout << "  ※ '访问哪个成员'变成运行期可变的 —— 通用序列化 / 按配置字段排序\n";

    std::cout << "\n=== ⑥ 空指针 -> 是 UB ===\n";
    Order* np = nullptr;
    std::cout << "  np == nullptr ? " << (np == nullptr) << "\n";
    std::cout << "  np->price → UB（多数实现段错误；无 MMU 的嵌入式上会读写地址 0 附近）\n";
    std::cout << "  正确做法：if (up) up->size();\n";

    delete p;
    return 0;
}
