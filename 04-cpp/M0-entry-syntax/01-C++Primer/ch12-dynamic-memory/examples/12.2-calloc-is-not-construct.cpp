// 12.2 直接内存管理 —— 可跑例子（三）
// 重点：calloc 的「清零」到底清出了什么，为什么它不等于构造对象
//
// 编译运行：
//   clang++ -std=c++20 -O0 12.2-calloc-is-not-construct.cpp -o /tmp/calloc_demo && /tmp/calloc_demo
//   clang++ -std=c++20 -O2 12.2-calloc-is-not-construct.cpp -o /tmp/calloc_demo_o2 && /tmp/calloc_demo_o2
//   /tmp/calloc_demo --crash        ← 复现「虚表指针被清零」后的 SIGSEGV（进程会崩，故意的）

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <new>
#include <type_traits>
#include <string_view>

// ① 有用户提供的构造函数：非 trivial，不是 implicit-lifetime type
struct A {
    int val;
    A() : val(42) {}
};

// ② 聚合体、trivial：是 implicit-lifetime type（C++20 起分配函数会隐式创建对象）
struct B {
    int val;
};

// ③ 带虚函数的类：内存里有个 vptr，被清零后虚调用会炸
struct V {
    int val;
    V() : val(42) {}
    virtual ~V() = default;
    virtual void say() const { std::printf("    V::say val=%d\n", val); }
};

// ④ 含指针 / 浮点成员：看「全零位」落上去是什么
struct Mixed {
    void* ptr;
    double d;
};

static void run_safe() {
    std::printf("========== 实验 1：calloc + 有构造函数的类 ==========\n");
    A* p = (A*)std::calloc(1, sizeof(A));
    std::printf("    构造函数写的是 val = 42\n");
    std::printf("    实际读到       val = %d     <- 读到 0 = 观测到不可能状态\n", p->val);
    std::printf("    is_trivially_default_constructible_v<A> = %d\n",
                (int)std::is_trivially_default_constructible_v<A>);
    std::printf("    结论：这块内存上没有 A 对象，访问成员 = UB\n");
    std::free(p);

    std::printf("\n========== 实验 2：calloc + trivial 聚合体 ==========\n");
    B* q = (B*)std::calloc(1, sizeof(B));
    std::printf("    val = %d\n", q->val);
    std::printf("    is_trivially_default_constructible_v<B> = %d\n",
                (int)std::is_trivially_default_constructible_v<B>);
    std::printf("    结论：B 是 implicit-lifetime type，C++20 起这样用是合法的\n");
    std::free(q);

    std::printf("\n========== 实验 3：正确做法（placement new + 显式析构）==========\n");
    void* raw = std::calloc(1, sizeof(A));
    A* r = new (raw) A();                 // 显式构造
    std::printf("    构造后 val = %d\n", r->val);
    std::printf("    r == raw ? %s\n", (void*)r == raw ? "是" : "否");
    r->~A();                              // 显式析构
    std::free(raw);                       // 只归还内存

    std::printf("\n========== 实验 4：全零位 ≠ 值为 0 ==========\n");
    Mixed* m = (Mixed*)std::calloc(1, sizeof(Mixed));
    std::printf("    ptr = %p   d = %g\n", m->ptr, m->d);
    std::printf("    （整数类型可以依赖全零位=0；指针/浮点只是实现上的巧合，不可依赖）\n");
    std::free(m);

    std::printf("\n========== 实验 5：0 字节分配 ==========\n");
    void* z1 = std::malloc(0);
    int*  z2 = new int[0];
    std::printf("    malloc(0)   = %s\n", z1 ? "非 null（唯一指针）" : "null（允许）");
    std::printf("    new int[0]  = %s\n", z2 ? "非 null（标准保证）" : "null");
    std::free(z1);
    delete[] z2;
}

static void run_crash() {
    std::printf("========== 实验 6：calloc 后调虚函数 ==========\n");
    std::printf("    sizeof(V) = %zu\n", sizeof(V));
    std::fflush(stdout);
    V* v = (V*)std::calloc(1, sizeof(V));
    std::printf("    val = %d，准备调用虚函数...\n", v->val);
    std::fflush(stdout);
    v->say();                              // vptr == 0 → SIGSEGV
    std::printf("    没崩（说明这个平台恰好扛住了）\n");
    std::free(v);
}

int main(int argc, char** argv) {
    if (argc > 1 && std::string_view{argv[1]} == "--crash") {
        run_crash();
    } else {
        run_safe();
        std::printf("\n（加 --crash 参数可复现虚表指针被清零导致的 SIGSEGV）\n");
    }
    return 0;
}
