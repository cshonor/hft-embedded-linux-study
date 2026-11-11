// 13.5 补充：type_traits 判定探针——「可赋值/可拷贝/可移动」编译期体检表
// 与 13.3-move-degradation.cpp（运行期日志 + 四分类 static_assert）互补，
// 本例覆盖它没讲的两件事：
//   ① 逐类型 × 逐 trait 的完整判定矩阵（构造与赋值分开看）
//   ② §二生成规则表没覆盖的维度：const 成员 / 引用成员废掉赋值（构造却安然无恙）
//
// 编译运行：
//   clang++ -std=c++20 13.5-type-traits-probe.cpp -o /tmp/d5 && /tmp/d5
// 实测：clang++ 23.1（macOS arm64, libc++）输出见笔记 §五 表格
#include <cstdio>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

// C++26 的 std::is_copyable / std::is_moveable（P3299）当前编译器还没内置，
// 按标准语义手工等价：
//   is_copyable_v<T>  = is_copy_constructible_v<T> && is_copy_assignable_v<T>
//   is_moveable_v<T>  = is_move_constructible_v<T> && is_move_assignable_v<T>
//                       && is_swappable_v<T>
template <class T>
inline constexpr bool my_is_copyable_v =
    std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>;

template <class T>
inline constexpr bool my_is_moveable_v =
    std::is_move_constructible_v<T> && std::is_move_assignable_v<T> &&
    std::is_swappable_v<T>;

// ---------- 受测类型 ----------
struct Trivial {};                       // 什么都不写：平凡全家桶

struct NoCopy {                          // 显式删拷贝 → 移动也不生成（§二规则）
    NoCopy(const NoCopy&) = delete;
    NoCopy& operator=(const NoCopy&) = delete;
};

struct ConstMember { const int x = 0; }; // const 成员：构造 ✅ 赋值 ❌

struct RefMember { int& r; };            // 引用成员：构造 ✅ 赋值 ❌（引用不可重绑）

struct DtorOnly { ~DtorOnly() {} };      // 只手写析构：移动不生成，move 构造退化走拷贝

// ---------- 判定矩阵 ----------
template <class T>
void report(const char* name) {
    std::printf("%-12s cC=%d cA=%d | mC=%d mA=%d | copyable=%d moveable=%d tcv=%d\n",
        name,
        (int)std::is_copy_constructible_v<T>,  // 拷贝构造（含 const& 吃右值的退化）
        (int)std::is_copy_assignable_v<T>,     // 拷贝赋值
        (int)std::is_move_constructible_v<T>,
        (int)std::is_move_assignable_v<T>,
        (int)my_is_copyable_v<T>,
        (int)my_is_moveable_v<T>,
        (int)std::is_trivially_copyable_v<T>); // memcpy 级拷贝（C struct 的现代等价）
}

int main() {
    std::printf("---- cC=拷贝构造 cA=拷贝赋值 mC=移动构造 mA=移动赋值 tcv=平凡可拷贝 ----\n");
    report<int>("int");
    report<const int>("const int");     // 顶层 const 废掉赋值（连 int 都不例外）
    report<std::string>("string");
    report<std::unique_ptr<int>>("unique_ptr");
    report<Trivial>("Trivial");
    report<NoCopy>("NoCopy");
    report<ConstMember>("ConstMember");
    report<RefMember>("RefMember");
    report<DtorOnly>("DtorOnly");
}
