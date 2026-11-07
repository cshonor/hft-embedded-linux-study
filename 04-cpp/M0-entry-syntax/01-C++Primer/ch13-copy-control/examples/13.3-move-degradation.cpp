// 13.3 补充：五个特殊成员函数 + 编译器生成规则实测
// 与本目录 13.2-rule-of-five.cpp（写全五个 vs 浅拷贝崩溃）互补，
// 本例覆盖它没讲的三件事：
//   ① 五个函数各自何时被调用（运行期打日志）
//   ② 声明析构 → 移动不再自动生成 → std::move 静默退化成拷贝（运行期可见）
//   ③ 四分类 + type traits 编译期验证（static_assert）
//
// 编译运行：
//   clang++ -std=c++20 13.3-move-degradation.cpp -o /tmp/d4 && /tmp/d4
// 实测：clang++ 23.1（macOS arm64）全部 static_assert 通过
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>

// ---------- ① 五个函数全写，各自打日志 ----------
struct Traced {
    std::string name;
    Traced(std::string n) : name(std::move(n)) {
        std::printf("  [构造]     %s\n", name.c_str());
    }
    Traced(const Traced& o) : name(o.name + "_copy") {
        std::printf("  [拷贝构造] %s <- %s\n", name.c_str(), o.name.c_str());
    }
    Traced& operator=(const Traced& o) {
        std::printf("  [拷贝赋值] %s <- %s\n", name.c_str(), o.name.c_str());
        return *this;
    }
    Traced(Traced&& o) noexcept : name(std::move(o.name)) {
        std::printf("  [移动构造] %s（只搬指针，~0 成本）\n", name.c_str());
    }
    Traced& operator=(Traced&& o) noexcept {
        name = std::move(o.name);
        std::printf("  [移动赋值] %s\n", name.c_str());
        return *this;
    }
    ~Traced() { std::printf("  [析构]     %s\n", name.c_str()); }
};

// ---------- ② 声明了析构（没声明移动）→ 移动操作不生成 ----------
struct HasDtor {
    std::string tag = "HasDtor";
    HasDtor() = default;
    HasDtor(const HasDtor&) { std::printf("  HasDtor: [拷贝构造] 被调用\n"); }
    ~HasDtor() { std::printf("  HasDtor: [析构]\n"); }
    // 没有移动构造 —— 因为声明了拷贝和析构，编译器不生成移动
};

// ---------- 全平凡类型对照（无任何用户声明，int 成员）----------
struct PlainOld { int x; };

// ---------- move-only：禁拷贝、留移动 ----------
struct MoveOnly {
    MoveOnly() = default;
    MoveOnly(const MoveOnly&)            = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&&)                 = default;
    MoveOnly& operator=(MoveOnly&&)      = default;
};

int main() {
    std::printf("== ① 五个函数何时被调用 ==\n");
    {
        Traced a("a");
        Traced b = a;              // 拷贝构造（新对象）
        b = a;                     // 拷贝赋值（已有对象）
        Traced c = std::move(a);   // 移动构造
        Traced d("d");
        d = std::move(c);          // 移动赋值
    }  // 出作用域：d c b a 逆序析构

    std::printf("\n== ② 声明析构后，std::move 退化成拷贝 ==\n");
    {
        HasDtor x;
        HasDtor y = std::move(x);  // 期望移动，实际调拷贝构造！
    }

    std::printf("\n== ③ 四分类编译期验证 ==\n");
    // 可拷可移
    static_assert(std::is_copy_constructible_v<std::string>);
    static_assert(std::is_move_constructible_v<std::string>);
    // move-only
    static_assert(!std::is_copy_constructible_v<std::unique_ptr<int>>);
    static_assert(std::is_move_constructible_v<std::unique_ptr<int>>);
    static_assert(!std::is_copy_constructible_v<MoveOnly>);
    static_assert(std::is_move_constructible_v<MoveOnly>);
    // 都不可
    static_assert(!std::is_copy_constructible_v<std::mutex>);
    static_assert(!std::is_move_constructible_v<std::mutex>);
    // 声明析构的影响：注意 is_move_constructible 仍是 true ——
    // 因为拷贝构造能吃右值（const& 绑定），「可移动构造」≠「生成了移动操作」
    static_assert(std::is_move_constructible_v<HasDtor>);       // true（拷贝兜底）
    static_assert(!std::is_trivially_move_constructible_v<HasDtor>);
    // 全平凡类型：移动是真·按位搬
    static_assert(std::is_trivially_move_constructible_v<PlainOld>);
    std::printf("  全部 static_assert 通过（编译期已验证）\n");
    return 0;
}
