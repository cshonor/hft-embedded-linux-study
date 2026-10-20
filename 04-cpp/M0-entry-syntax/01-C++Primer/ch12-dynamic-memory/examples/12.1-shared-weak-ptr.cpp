// 12.1 shared_ptr / weak_ptr 深入：控制块、enable_shared_from_this、别名构造、缓存
//
// 用法：
//   clang++ -std=c++20 12.1-shared-weak-ptr.cpp -o /tmp/sw && /tmp/sw
//   /tmp/sw 1    只跑第 1 段（不传则跑除"崩溃演示"外的全部）
//   /tmp/sw crash  跑第 2 段的双控制块崩溃演示（会 SIGABRT，验证后再跑）
//
// 崩溃演示默认跳过，因为它们故意触发未定义行为。

#include <memory>
#include <unordered_map>
#include <string>
#include <cstdio>

// ---------- 1. 删除器：shared_ptr 类型擦除，unique_ptr 不擦除 ----------
static void demo1() {
    printf("== 1. 删除器与 sizeof ==\n");
    struct BigDeleter { char junk[64]; void operator()(int* p) const { delete p; } };
    auto s1 = std::shared_ptr<int>(new int(1), [](int* p) { delete p; });
    auto s3 = std::shared_ptr<int>(new int(3), BigDeleter{});
    printf("  sizeof(shared_ptr<int> 配 lambda 删除器)      = %zu\n", sizeof(s1));
    printf("  sizeof(shared_ptr<int> 配 64 字节仿函数删除器) = %zu  <- 一样大\n", sizeof(s3));
    printf("  sizeof(unique_ptr<int>)                      = %zu\n", sizeof(std::unique_ptr<int>));
    printf("  sizeof(unique_ptr<int, BigDeleter>)          = %zu  <- 删除器撑大了对象\n",
           sizeof(std::unique_ptr<int, BigDeleter>));
}

// ---------- 2. 双控制块 → 析构两次 / SIGABRT ----------
struct T { int v; ~T() { printf("    [析构] T(%d)\n", v); } };
static void demo_crash() {
    printf("== 2. 同一个裸指针 → 两个控制块（故意触发 UB）==\n");
    T* raw = new T{42};
    {
        std::shared_ptr<T> a(raw);
        std::shared_ptr<T> b(raw);            // 第二个控制块
        printf("  a.use_count=%ld  b.use_count=%ld  (各自为 1)\n", a.use_count(), b.use_count());
    }
    printf("  ↑ [析构] 打了两遍 = delete 两次；实际工程里常表现为 SIGABRT(134)\n");
}

// ---------- 3. enable_shared_from_this ----------
static void demo3() {
    printf("== 3. enable_shared_from_this ==\n");
    struct Good : std::enable_shared_from_this<Good> {
        ~Good() { printf("    [析构] Good\n"); }
        std::shared_ptr<Good> self() { return shared_from_this(); }
    };
    {
        auto p = std::make_shared<Good>();
        auto q = p->self();
        printf("  p.use_count=%ld q.use_count=%ld（同一控制块）\n", p.use_count(), q.use_count());
    }
    struct Early : std::enable_shared_from_this<Early> {
        Early() {
            try { (void)shared_from_this(); }
            catch (const std::bad_weak_ptr& e) { printf("  构造函数内抛 bad_weak_ptr: %s\n", e.what()); }
        }
    };
    { auto p = std::make_shared<Early>(); (void)p; }
    Good stack_obj;                                   // 栈对象
    try { (void)stack_obj.self(); }
    catch (const std::bad_weak_ptr& e) { printf("  栈对象上调 shared_from_this() 抛: %s\n", e.what()); }
}

// ---------- 4. 别名构造 / 数组 / unique_ptr → shared_ptr ----------
static void demo4() {
    printf("== 4. 别名构造与其他转换 ==\n");
    struct Outer { int a = 1; int b = 2; ~Outer() { printf("    [析构] Outer\n"); } };
    {
        std::shared_ptr<int> pb;
        {
            auto po = std::make_shared<Outer>();
            pb = std::shared_ptr<int>(po, &po->b);    // 指向成员，共享 Outer 的控制块
            printf("  *pb=%d，Outer.use_count=%ld\n", *pb, po.use_count());
        }
        printf("  po 已出作用域，*pb=%d 仍有效（Outer 被 pb 保活）\n", *pb);
    }
    {
        auto arr = std::make_shared<int[]>(5);        // C++20 起 make_shared 支持数组
        for (int i = 0; i < 5; ++i) arr[i] = i * 10;
        printf("  make_shared<int[]>(5): arr[3]=%d\n", arr[3]);
    }
    {
        auto up = std::make_unique<Outer>();
        std::shared_ptr<Outer> sp = std::move(up);    // 此时才建控制块
        printf("  unique_ptr → shared_ptr：up 为空? %s, use_count=%ld\n",
               up ? "否" : "是", sp.use_count());
    }
}

// ---------- 5. weak_ptr：对象死了控制块还在 ----------
static void demo5() {
    printf("== 5. weak_ptr 与控制块的寿命 ==\n");
    struct Outer { ~Outer() { printf("    [析构] Outer（对象先走）\n"); } };
    std::weak_ptr<Outer> w;
    {
        auto sp = std::make_shared<Outer>();
        w = sp;
        printf("  作用域内：expired=%d use_count=%ld\n", (int)w.expired(), w.use_count());
    }
    printf("  作用域外：expired=%d，lock() 返回空? %s\n",
           (int)w.expired(), w.lock() ? "否" : "是");
    printf("  对象已析构，但控制块还活着 —— 这正是 make_shared 内存延迟归还的原因\n");
}

// ---------- 6. weak_ptr 缓存：复用但不延长生命周期 ----------
static void demo6() {
    printf("== 6. 缓存场景：容器只存 weak_ptr ==\n");
    struct Snapshot {
        std::string symbol; long seq;
        Snapshot(std::string s, long q) : symbol(std::move(s)), seq(q) {
            printf("    [构造] 快照 %s#%ld\n", symbol.c_str(), seq); }
        ~Snapshot() { printf("    [析构] 快照 %s#%ld\n", symbol.c_str(), seq); }
    };
    class SnapshotCache {
        std::unordered_map<std::string, std::weak_ptr<Snapshot>> cache_;
    public:
        std::shared_ptr<Snapshot> get(const std::string& sym, long seq) {
            if (auto it = cache_.find(sym); it != cache_.end())
                if (auto hit = it->second.lock()) {
                    printf("  [命中缓存] %s#%ld\n", sym.c_str(), hit->seq);
                    return hit;
                }
            printf("  [缓存条目已失效] %s\n", sym.c_str());
            auto fresh = std::make_shared<Snapshot>(sym, seq);
            cache_[sym] = fresh;                     // 只存弱引用
            return fresh;
        }
    };
    SnapshotCache c;
    printf("  -- 第一次取 --\n");
    auto a = c.get("600519", 1);
    printf("  -- 第二次取同一标的（应命中）--\n");
    auto b = c.get("600519", 1);
    printf("  同一对象? %s\n", a.get() == b.get() ? "是" : "否");
    printf("  -- 外部全部释放后再取（应重建）--\n");
    a.reset(); b.reset();
    auto d = c.get("600519", 2);
    (void)d;
}

int main(int argc, char** argv) {
    std::string sel = argc > 1 ? argv[1] : "all";
    if (sel == "crash") { demo_crash(); return 0; }
    if (sel == "all" || sel == "1") demo1();
    if (sel == "all" || sel == "3") demo3();
    if (sel == "all" || sel == "4") demo4();
    if (sel == "all" || sel == "5") demo5();
    if (sel == "all" || sel == "6") demo6();
    if (sel == "all")
        printf("\n（第 2 段是故意的 UB 演示，用 `%s crash` 单独跑，会 SIGABRT）\n", argv[0]);
    return 0;
}
