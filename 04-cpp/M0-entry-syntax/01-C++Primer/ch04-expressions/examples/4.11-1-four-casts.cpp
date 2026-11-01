// 4.11-1 四种命名 cast 的边界实测
//
// 编译：
//   clang++ -std=c++17 -O2 -Wall examples/4.11-1-four-casts.cpp -o casts && ./casts
//
// 要点：C++ 的四种 cast 把 C 的「一锅烩」(T)x 拆成四个意图明确的动作。

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <typeinfo>

struct Base {
    virtual ~Base() = default;      // 有虚函数 = 多态类型，dynamic_cast 才能用
    int b = 1;
};
struct Derived : Base {
    int d = 2;
};
struct Other : Base {
    int o = 3;
};

int main() {
    printf("=== 4.11-1 四种 cast 实测 ===\n\n");

    // ---------- [1] static_cast：编译期、可反转、但不能去 const ----------
    printf("[1] static_cast\n");
    int i = 42;
    double d = static_cast<double>(i);
    printf("    int -> double : %d -> %.1f\n", i, d);
    printf("    double -> int : 3.99 -> %d   (向零截断，不是四舍五入)\n", static_cast<int>(3.99));
    printf("    double -> int : -3.99 -> %d  (负数也是向零：不是 -4)\n", static_cast<int>(-3.99));
    void* vp = static_cast<void*>(&i);
    printf("    int* -> void* -> int* : %d   (void* 只能靠 static_cast 回来)\n",
           *static_cast<int*>(vp));

    Derived der;
    Base* bp = &der;
    // 向下转型：static_cast 不做运行时检查，用错了就是 UB
    Derived* dp_static = static_cast<Derived*>(bp);   // 这里碰巧是对的
    printf("    Base* -> Derived* (static_cast，不检查) : d = %d\n", dp_static->d);
    Other oth;
    Base* bp2 = &oth;
    Derived* wrong = static_cast<Derived*>(bp2);      // ❌ 编译通过，运行时 UB
    printf("    把 Other* 当 Derived* 用（static_cast 不拦）：读到 o 的位置 = %d\n", wrong->d);
    printf("    ⚠ 这就是 static_cast 向下转型的危险：编译无错、运行无警告\n\n");

    // ---------- [2] const_cast：唯一能增删 const/volatile ----------
    printf("[2] const_cast\n");
    const int ci = 10;
    const int* cip = &ci;
    int* mp = const_cast<int*>(cip);
    printf("    去掉 const 前：ci = %d，拿到可写指针 mp = %s\n", ci, mp == &ci ? "指向 ci" : "?");
    // *mp = 20;   // ❌ UB：修改「原本就是 const」的对象
    // 上面这行在某些编译器/优化下会崩（常量放 .rodata），在某些下「看起来成功」。
    // 注释掉以保证本程序可跑；想看 UB 的后果请自行打开并用 -O0/-O2 分别试。
    printf("    *mp = 20 是 UB（对象本身是 const），本例故意不执行\n");

    // const_cast 的正当用途：对接「不修改对象但签名没有 const」的老接口
    int real = 7;
    const int* rp = &real;
    int* rmp = const_cast<int*>(rp);
    *rmp = 9;                                          // ✅ 对象本身不是 const，合法
    printf("    对象本身非 const 时，去 const 后写入是合法的：real = %d\n", real);
    printf("    判断标准：被指向的**对象**是不是 const，不是指针是不是 const\n\n");

    // ---------- [3] reinterpret_cast：比特重解释，可移植性最差 ----------
    printf("[3] reinterpret_cast\n");
    uint64_t addr = reinterpret_cast<uint64_t>(&i);
    printf("    int* -> uint64_t : 0x%llx   (嵌入式里访问 MMIO 寄存器常用这招)\n",
           (unsigned long long)addr);
    printf("    再转回 int*      : %d\n", *reinterpret_cast<int*>(addr));
    printf("    ⚠ 指针->整数->指针在 64 位下必须用 uint64_t，用 int 会截断\n");

    // 类型双关：错误做法 vs 正确做法
    float f = 1.0f;
    uint32_t raw_bad;
    // 错误：违反严格别名规则（strict aliasing），-O2 下的结果不可信
    // raw_bad = *reinterpret_cast<uint32_t*>(&f);
    std::memcpy(&raw_bad, &f, sizeof raw_bad);         // ✅ 正确做法
    printf("    float 1.0f 的位模式（memcpy）= 0x%08x\n", raw_bad);
    printf("    reinterpret_cast 直接读是 UB；要看位模式一律用 memcpy\n\n");

    // ---------- [4] dynamic_cast：唯一有运行时检查的 ----------
    printf("[4] dynamic_cast\n");
    Base* pD = &der;
    Base* pO = &oth;
    printf("    成功：dynamic_cast<Derived*>(指向 Derived 的 Base*) -> %s\n",
           dynamic_cast<Derived*>(pD) ? "非空" : "nullptr");
    printf("    失败：dynamic_cast<Derived*>(指向 Other 的 Base*)   -> %s\n",
           dynamic_cast<Derived*>(pO) ? "非空" : "nullptr");
    printf("    失败时指针版返回 nullptr，引用版抛 std::bad_cast：\n");
    try {
        Base& refO = oth;
        (void)dynamic_cast<Derived&>(refO);
    } catch (const std::bad_cast& e) {
        printf("        catch std::bad_cast : %s\n", e.what());
    }
    printf("    ⚠ 前提：Base 必须有虚函数（多态类型），否则编译报错\n\n");

    // ---------- [5] dynamic_cast 的代价 ----------
    printf("[5] dynamic_cast 的开销（1e7 次成功转型）\n");
    const int N = 10000000;
    volatile int sink = 0;

    auto t0 = std::chrono::steady_clock::now();
    for (int k = 0; k < N; ++k) {
        if (dynamic_cast<Derived*>(pD)) ++sink;
    }
    auto t1 = std::chrono::steady_clock::now();

    auto t2 = std::chrono::steady_clock::now();
    for (int k = 0; k < N; ++k) {
        if (static_cast<Derived*>(pD)) ++sink;
    }
    auto t3 = std::chrono::steady_clock::now();

    auto t4 = std::chrono::steady_clock::now();
    for (int k = 0; k < N; ++k) {
        sink += pD->b;                                  // 纯虚调用（通过对象，无开销）
    }
    auto t5 = std::chrono::steady_clock::now();

    double dyn  = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double stat = std::chrono::duration<double, std::milli>(t3 - t2).count();
    double virt = std::chrono::duration<double, std::milli>(t5 - t4).count();
    printf("    dynamic_cast : %7.2f ms  (%.2f ns/次)\n", dyn, dyn * 1e6 / N);
    printf("    static_cast  : %7.2f ms  (%.2f ns/次)  —— 编译期，零成本\n", stat, stat * 1e6 / N);
    printf("    成员访问     : %7.2f ms  (%.2f ns/次)\n", virt, virt * 1e6 / N);
    printf("    结论：热路径上别用 dynamic_cast；需要向下转型先想清楚设计\n\n");

    // ---------- [6] 速查表 ----------
    printf("[6] 四种 cast 速查\n");
    printf("    ┌───────────────────┬──────────────┬────────┬──────────────────────┐\n");
    printf("    │ cast              │ 用途         │ 检查   │ 典型风险             │\n");
    printf("    ├───────────────────┼──────────────┼────────┼──────────────────────┤\n");
    printf("    │ static_cast       │ 常规转换     │ 编译期 │ 向下转型不检查       │\n");
    printf("    │ const_cast        │ 增删 const   │ 编译期 │ 改真 const 对象是 UB │\n");
    printf("    │ reinterpret_cast  │ 比特重解释   │ 无     │ 严格别名 / 不可移植  │\n");
    printf("    │ dynamic_cast      │ 多态向下转型 │ 运行时 │ 慢，且需 RTTI        │\n");
    printf("    └───────────────────┴──────────────┴────────┴──────────────────────┘\n");
    printf("    C 风格 (T)x 会按顺序尝试以上全部（甚至包括 const_cast）——意图不可读，禁用\n");

    return 0;
}
