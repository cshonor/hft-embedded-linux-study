// 12.1 weak_ptr：循环引用泄漏 → weak_ptr 修复（父子节点，最经典的写法）
//
// 编译运行：
//   clang++ -std=c++17 12.1-weak-ptr-cycle.cpp -o /tmp/cyc && /tmp/cyc
//
// 想要"泄漏了多少字节"的硬证据，用 LeakSanitizer：
//   clang++ -std=c++17 -fsanitize=address -g 12.1-weak-ptr-cycle.cpp -o /tmp/cyc_asan
//   ASAN_OPTIONS=detect_leaks=1 /tmp/cyc_asan
//   -> SUMMARY: AddressSanitizer: NNN byte(s) leaked in M allocation(s).
//
// 内存中还有一段故意触发 UB 的多线程演示，默认不跑：`/tmp/cyc race`（会 SIGSEGV）

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

// ---- 分配/归还计数：比"看析构有没有打印"更硬 ----
static long g_alloc = 0, g_free = 0;
static bool g_count = false;
void* operator new(size_t n) {
    void* p = std::malloc(n ? n : 1);
    if (g_count && p) ++g_alloc;
    return p;
}
void operator delete(void* p) noexcept { if (p) { if (g_count) ++g_free; std::free(p); } }
void operator delete(void* p, size_t) noexcept { if (p) { if (g_count) ++g_free; std::free(p); } }

// ============ 泄漏版：父子都用 shared_ptr ============
namespace bad {
struct Child;
struct Parent {
    std::string name;
    std::vector<std::shared_ptr<Child>> children;      // 父拥有子
    explicit Parent(std::string n) : name(std::move(n)) {
        std::printf("    [构造] Parent(%s)\n", name.c_str()); }
    ~Parent() { std::printf("    [析构] Parent(%s)\n", name.c_str()); }
};
struct Child {
    std::string name;
    std::shared_ptr<Parent> parent;                    // ❌ 回指父也用 shared_ptr → 成环
    explicit Child(std::string n) : name(std::move(n)) {
        std::printf("    [构造] Child(%s)\n", name.c_str()); }
    ~Child() { std::printf("    [析构] Child(%s)\n", name.c_str()); }
};
} // namespace bad

// ============ 修复版：子用 weak_ptr 回看父 ============
namespace good {
struct Child;
struct Parent {
    std::string name;
    std::vector<std::shared_ptr<Child>> children;      // 父拥有子：shared_ptr
    explicit Parent(std::string n) : name(std::move(n)) {
        std::printf("    [构造] Parent(%s)\n", name.c_str()); }
    ~Parent() { std::printf("    [析构] Parent(%s)\n", name.c_str()); }
};
struct Child {
    std::string name;
    std::weak_ptr<Parent> parent;                      // ✅ 子回看父：weak_ptr
    explicit Child(std::string n) : name(std::move(n)) {
        std::printf("    [构造] Child(%s)\n", name.c_str()); }
    ~Child() { std::printf("    [析构] Child(%s)\n", name.c_str()); }
    void who_is_my_parent() const {                    // 用之前必须 lock()
        if (auto p = parent.lock())
            std::printf("    %s 的父节点是 %s（use_count=%ld）\n",
                        name.c_str(), p->name.c_str(), p.use_count());
        else
            std::printf("    %s 的父节点已经不在了\n", name.c_str());
    }
};
} // namespace good

static void demo_leak() {
    std::printf("== 1. 循环引用：父子互相 shared_ptr（泄漏）==\n");
    g_alloc = 0; g_free = 0; g_count = true;
    {
        auto parent = std::make_shared<bad::Parent>("root");
        auto child  = std::make_shared<bad::Child>("leaf");
        parent->children.push_back(child);
        child->parent = parent;                        // 环闭合
        std::printf("  parent.use_count=%ld  child.use_count=%ld\n",
                    parent.use_count(), child.use_count());
    }
    g_count = false;
    std::printf("  ↑ 没有任何 [析构] —— 计数归不了零，泄漏\n");
    std::printf("  分配 %ld 次，归还 %ld 次（差 %ld）\n\n", g_alloc, g_free, g_alloc - g_free);
}

static void demo_fixed() {
    std::printf("== 2. 子用 weak_ptr 回看父（修复）==\n");
    g_alloc = 0; g_free = 0; g_count = true;
    {
        auto parent = std::make_shared<good::Parent>("root");
        auto child  = std::make_shared<good::Child>("leaf");
        parent->children.push_back(child);
        child->parent = parent;                        // 弱引用，不形成环
        std::printf("  parent.use_count=%ld  child.use_count=%ld（父仍是 1）\n",
                    parent.use_count(), child.use_count());
        child->who_is_my_parent();                     // lock() 成功
    }
    g_count = false;
    std::printf("  ↑ [析构] 都打了 —— 计数能归零\n");
    std::printf("  分配 %ld 次，归还 %ld 次（差 %ld）\n\n", g_alloc, g_free, g_alloc - g_free);
}

static void demo_expired() {
    std::printf("== 3. 父节点先走，子节点的 weak_ptr 自动失效 ==\n");
    std::weak_ptr<good::Parent> watcher;
    {
        auto parent = std::make_shared<good::Parent>("short-lived");
        auto child  = std::make_shared<good::Child>("leaf2");
        child->parent = parent;
        watcher = child->parent;
        child->who_is_my_parent();                     // 还活着
        child.reset();                                 // 注意：这里只放 child
    }
    std::printf("  离开作用域后：expired=%d，lock() 为空? %s\n\n",
                (int)watcher.expired(), watcher.lock() ? "否" : "是");
}

// 多线程下 expired() 与 lock() 之间的窗口（故意触发 UB）
static void demo_race() {
    std::printf("== 4. 多线程：expired() 之后再用 lock() 有窗口（会崩）==\n");
    static volatile long sink = 0;
    for (int round = 0; round < 50; ++round) {
        auto sp = std::make_shared<int>(7);
        std::weak_ptr<int> w = sp;
        std::atomic<bool> go{false};
        std::thread killer([&] { while (!go) {} sp.reset(); });
        std::thread user([&] {
            while (!go) {}
            for (int i = 0; i < 200000; ++i) {
                std::shared_ptr<int> s = w.lock();
                sink += *s;                            // 不判空 → 空指针解引用
            }
        });
        go = true;
        killer.join(); user.join();
    }
    std::printf("  没崩（本轮运气好）\n");
}

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "race") { demo_race(); return 0; }
    demo_leak();
    demo_fixed();
    demo_expired();
    std::printf("（第 4 段多线程崩溃演示用 `%s race` 单独跑）\n", argv[0]);
    return 0;
}
