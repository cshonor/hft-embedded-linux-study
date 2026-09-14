// 13.1 拷贝控制成员 —— 可跑例子
// 打印每个特殊成员函数的调用，看清「什么时候调用谁」
//
// 编译运行：
//   clang++ -std=c++20 13.1-copy-control-members.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>
#include <utility>
#include <vector>

struct Tracked {
    int id;

    static int n_ctor, n_copy, n_move, n_copy_asgn, n_move_asgn, n_dtor;

    Tracked(int i = 0) : id(i) {
        ++n_ctor;
        std::printf("    [构造]     id=%d\n", id);
    }
    Tracked(const Tracked &o) : id(o.id) {
        ++n_copy;
        std::printf("    [拷贝构造] id=%d\n", id);
    }
    Tracked(Tracked &&o) noexcept : id(o.id) {
        ++n_move;
        o.id = -1;
        std::printf("    [移动构造] id=%d（源对象被置为 -1）\n", id);
    }
    Tracked &operator=(const Tracked &o) {
        ++n_copy_asgn;
        id = o.id;
        std::printf("    [拷贝赋值] id=%d\n", id);
        return *this;
    }
    Tracked &operator=(Tracked &&o) noexcept {
        ++n_move_asgn;
        id = o.id;
        o.id = -1;
        std::printf("    [移动赋值] id=%d\n", id);
        return *this;
    }
    ~Tracked() {
        ++n_dtor;
        std::printf("    [析构]     id=%d\n", id);
    }
};
int Tracked::n_ctor = 0;
int Tracked::n_copy = 0;
int Tracked::n_move = 0;
int Tracked::n_copy_asgn = 0;
int Tracked::n_move_asgn = 0;
int Tracked::n_dtor = 0;

int main() {
    {
        std::printf("--- 1. 直接构造 ---\n");
        Tracked a(1);

        std::printf("\n--- 2. 拷贝构造（b 从 a 来）---\n");
        Tracked b = a;

        std::printf("\n--- 3. 移动构造（std::move）---\n");
        Tracked c = std::move(a);
        std::printf("    a.id 现在是 %d\n", a.id);

        std::printf("\n--- 4. 拷贝赋值 ---\n");
        Tracked d(2);
        d = b;

        std::printf("\n--- 5. 移动赋值 ---\n");
        Tracked e(3);
        e = std::move(d);

        std::printf("\n--- 6. vector push_back：临时对象进容器 ---\n");
        {
            std::vector<Tracked> v;
            v.reserve(4);
            for (int i = 10; i < 12; i++) {
                std::printf("  push_back(Tracked(%d)):\n", i);
                v.push_back(Tracked(i));
            }
        }
        std::printf("\n--- 7. 全部对象在此析构 ---\n");
    }

    std::printf("\n=== 统计 ===\n");
    std::printf("  构造     = %d\n", Tracked::n_ctor);
    std::printf("  拷贝构造 = %d\n", Tracked::n_copy);
    std::printf("  移动构造 = %d\n", Tracked::n_move);
    std::printf("  拷贝赋值 = %d\n", Tracked::n_copy_asgn);
    std::printf("  移动赋值 = %d\n", Tracked::n_move_asgn);
    std::printf("  析构     = %d\n", Tracked::n_dtor);

    int created = Tracked::n_ctor + Tracked::n_copy + Tracked::n_move;
    std::printf("\n  创建对象数 = %d，析构数 = %d  -> %s\n", created, Tracked::n_dtor,
                (created == Tracked::n_dtor) ? "配平，没有泄漏" : "不配平！");
    return 0;
}
