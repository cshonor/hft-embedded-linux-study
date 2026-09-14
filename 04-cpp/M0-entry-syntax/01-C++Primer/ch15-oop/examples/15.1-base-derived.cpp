// 15.1 基类与派生类 —— 派生对象里基子物排在最前面
// 实测：clang++ -std=c++20 -Wall 15.1-base-derived.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>

class FeedHandler {                   // 基类：行情公共部分
public:
    explicit FeedHandler(long id) : feed_id_(id) {}
    long feed_id() const { return feed_id_; }
protected:
    // protected：派生类可以访问，外部不行（比 private 松一档）
    long feed_id_;
};

class Level2Feed : public FeedHandler {   // 派生类：在公共部分上加深度档位
public:
    Level2Feed(long id, int levels) : FeedHandler(id), levels_(levels) {}
    void describe() const {
        // 派生类成员函数里可以直接摸基类的 protected 成员
        printf("  feed=%ld, 深度 %d 档\n", feed_id_, levels_);
    }
    // 供 main 里做布局测量用（外部拿不到 protected 成员，绕道成员函数）
    const void* self()      const { return this; }
    const void* base_addr() const { return static_cast<const FeedHandler*>(this); }
    const void* levels_addr() const { return &levels_; }
private:
    int levels_;    // 追加在基子物之后
};

int main() {
    Level2Feed f(10086, 10);
    f.describe();

    // 内存布局验证：派生对象 = 基子物 + 派生新增成员
    printf("\nsizeof(FeedHandler) = %zu\n", sizeof(FeedHandler));
    printf("sizeof(Level2Feed)  = %zu\n", sizeof(Level2Feed));

    Level2Feed g(7, 5);
    auto diff = [](const void* a, const void* b) -> long {
        return (const char*)a - (const char*)b;
    };
    printf("\n&g                          = %p\n", g.self());
    printf("g 内的基子物地址             = %p  (偏移 %ld 字节)\n",
           g.base_addr(), diff(g.base_addr(), g.self()));
    printf("g 内的 levels_ 地址          = %p  (偏移 %ld 字节)\n",
           g.levels_addr(), diff(g.levels_addr(), g.self()));
    // 基子物偏移 = 0：派生对象的开头就是基子物

    // 这个"开头重合"不是巧合，是派生到基转换零成本的物理基础：
    // Level2Feed* -> FeedHandler* 就是数值不变的指针（单继承场景）
    // 代价是编译期已知。到了 15.2 加上 virtual，会看到第二个条件。
    return 0;
}
