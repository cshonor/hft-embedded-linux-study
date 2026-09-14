// 7.4 类的其他特性 —— mutable 成员、成员初始化的真实顺序
// 实测：clang++ -std=c++20 -Wall 7.4-mutable-and-init-order.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>

// ---------- mutable：const 函数里唯一可以改的东西 ----------
class Cache {
public:
    // 查询是"只读"操作 -> 标 const。但命中计数必须累加，
    // 于是把 count_ 标成 mutable：const 函数里唯独它可写。
    long get(int key) const {
        ++hits_;                    // 合法：hits_ 是 mutable
        return key * 2;
    }
    long hits() const { return hits_; }
private:
    mutable long hits_ = 0;         // "逻辑只读、物理会变"的统计量
};

// ---------- 成员初始化的真实顺序 = 声明顺序（与初始化列表书写顺序无关） ----------
static int probe(const char* name) {
    printf("    初始化 %-4s\n", name);
    return 0;
}

class Widget {
public:
    // 列表里故意先写 b_ 再写 a_ —— 但实际执行顺序看下面的输出
    Widget() : b_(probe("b_")), a_(probe("a_")) {}
private:
    int a_;    // 声明在前 -> 先初始化
    int b_;    // 声明在后 -> 后初始化
};

int main() {
    Cache c;
    c.get(1); c.get(2); c.get(3);
    printf("const 成员函数累计命中 %ld 次\n", c.hits());

    printf("\n构造 Widget（初始化列表里 b_ 写在 a_ 前面）:\n");
    Widget w;
    // 输出是 a_ 先、b_ 后 —— 因为 a_ 声明在前。
    // 为什么这条纪律重要：如果 b_ 的初值依赖 a_（比如 b_(a_+1)），
    // 而声明顺序相反，b_ 拿到的是未初始化的 a_ —— 埋一颗随机炸弹。
    printf("\n纪律: 初始化列表的书写顺序必须与成员声明顺序一致，\n");
    printf("      -Wreorder 会替你盯住这件事。\n");
    return 0;
}
