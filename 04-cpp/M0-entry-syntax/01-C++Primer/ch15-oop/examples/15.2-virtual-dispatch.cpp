// 15.2 虚函数与动态绑定 —— 亲手摸到 vptr 和虚表
// 实测：clang++ -std=c++20 -Wall 15.2-virtual-dispatch.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>
#include <cstring>
#include <cstdint>

// ---------- 非虚版本：没有 vptr，调用在编译期就定死 ----------
struct FeedNV {
    long    id;
    const char* name() const { return "FeedNV::name"; }   // 非虚
};

// ---------- 虚版本：多出一个 vptr ----------
struct Feed {
    explicit Feed(long i) : id(i) {}
    long    id;
    virtual const char* name() const { return "Feed::name"; }
    virtual const char* codec() const { return "binary"; }
};

struct FasterFeed : Feed {
    explicit FasterFeed(long i) : Feed(i) {}
    // 覆盖 name()，不覆盖 codec()
    const char* name() const override { return "FasterFeed::name"; }
};

// 通过对象地址把前 8 个字节读出来 —— arm64 小端机器上那正是 vptr
static uintptr_t vptr_of(const void* obj) {
    uintptr_t v;
    std::memcpy(&v, obj, sizeof(v));
    return v;
}

int main() {
    printf("sizeof(FeedNV) = %zu（无虚函数：只有数据）\n", sizeof(FeedNV));
    printf("sizeof(Feed)   = %zu（有虚函数：数据 + 8 字节 vptr）\n\n", sizeof(Feed));

    Feed        a{1};
    FasterFeed  b{2};

    printf("a 的 vptr = 0x%lx  -> 指向 Feed 的虚表\n", vptr_of(&a));
    printf("b 的 vptr = 0x%lx  -> 指向 FasterFeed 的虚表\n", vptr_of(&b));
    printf("两个 vptr %s\n",
           vptr_of(&a) == vptr_of(&b) ? "相同（不该发生）" : "不同（各自类的虚表）");
    // 同一类型所有对象共享一张虚表 -> vptr 相同；类型不同 -> 表不同

    // ---------- 动态绑定：调用发生在运行期 ----------
    Feed* p = &b;                     // 静态类型 Feed*，动态类型 FasterFeed
    printf("\n经基类指针调用 name():   %s  <- 看的是动态类型（虚）\n", p->name());
    printf("经基类指针调用 codec():  %s  <- 未覆盖，回落基类版本\n", p->codec());

    // 编译器生成的调用形态（伪码）：
    //   p->vptr->slot[name()]  —— 先取对象头 8 字节 vptr，再查表跳转
    // 对比非虚调用：直接 call 固定地址，一次都不用查。
    // 这就是虚调用比直接调用多两次内存访问的原因，
    // 也是分支预测器 cache 不友好场景（如乱序行情流）慎用虚函数的原因。
    printf("\n代价: 虚调用 = 取 vptr + 查表 + 间接跳转，且阻断内联。\n");
    return 0;
}
