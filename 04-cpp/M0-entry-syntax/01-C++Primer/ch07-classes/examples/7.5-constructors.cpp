// 7.5 构造函数 —— 委托构造、const/引用成员只能用初始化列表
// 实测：clang++ -std=c++20 -Wall 7.5-constructors.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>

class Feed {
public:
    // 主构造：干全部的活
    Feed(const char* name, long id) : name_(name), id_(id) {
        printf("  [Feed(%s, %ld)] 真正干活的构造\n", name_, id_);
    }
    // 委托构造：自己不干活，转手交给主构造（避免初始化逻辑复制粘贴）
    explicit Feed(const char* name) : Feed(name, 0) {
        printf("  [Feed(%s)] 委托完毕，补点自己的事\n", name_);
    }
    const char* name() const { return name_; }
private:
    const char* name_;
    long id_;
};

// const 成员和引用成员：构造函数体内"赋值"是非法的，必须在初始化列表里绑定
class Order {
public:
    // 正确：初始化列表一次性绑定
    Order(long id, long& book_ref)
        : id_(id), book_(book_ref), qty_(0) {
        printf("  [Order] id=%ld 绑定订单簿引用\n", id_);
    }
    // 错误写法（编译不通过，可解注释体验）：
    // Order(long id, long& book_ref) {
    //     id_ = id;        // error: assignment of read-only member 'id_'
    //     book_ = book_ref; // error: book_ 未初始化就使用（引用不能重新绑定）
    // }
private:
    const long id_;     // const 成员
    long&       book_;  // 引用成员
    long        qty_;
};

int main() {
    printf("构造序列:\n");
    Feed f1("AAPL");                  // 委托 -> 主构造 -> 补充逻辑
    printf("\n");
    Feed f2("MSFT", 10086);           // 直接走主构造

    printf("\n");
    long book = 0;
    Order o(42, book);
    // 输出验证了三件事：
    // 1. 委托构造 = 只有一个入口干初始化，其余构造"转发"
    // 2. const / 引用成员只能且必须在初始化列表绑定，函数体里"赋值"非法
    // 3. 初始化列表不是"先构造再赋值"，而是一步到位（对引用/const 是唯一途径）
    return 0;
}
