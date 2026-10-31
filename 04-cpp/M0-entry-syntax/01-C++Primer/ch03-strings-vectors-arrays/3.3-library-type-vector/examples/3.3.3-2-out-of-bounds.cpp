// 3.3.3-2  越界访问：[] 是 UB，at() 抛异常
//
// 编译（普通）：clang++ -std=c++17 -O2 3.3.3-2-out-of-bounds.cpp -o oob2 && ./oob2
// 编译（ASan）：clang++ -std=c++17 -O1 -g -fsanitize=address 3.3.3-2-out-of-bounds.cpp -o oob2 && ./oob2
//
// ASan 输出（关键行）：
//   ERROR: AddressSanitizer: heap-buffer-overflow
//   READ of size 4 at 0x602000000980
//   0x602000000980 is located 0 bytes after 16-byte region [0x602000000970, 0x602000000980)
//                                          ^^^^^^^^^^^^^^^
//   越界地址正好落在分配块末尾之外 0 字节 —— 读到的是【隔壁堆块】的内容，
//   地址合法、进程内可读，所以 release 下【不崩溃】，只是值是垃圾。

#include <iostream>
#include <stdexcept>
#include <vector>

int main() {
    std::vector<int> v(4);          // 合法下标 0..3，分配 16 字节
    std::cout << "vector<int> v(4)：合法下标 0..3，缓冲区 16 字节\n\n";

    // ① at() 做检查 → 抛异常
    std::cout << "① v.at(99999)：\n";
    try {
        std::cout << "   " << v.at(99999) << "\n";
    } catch (const std::out_of_range& e) {
        std::cout << "   抛出 std::out_of_range: " << e.what() << "\n";
    }

    // ② [] 不检查 → UB
    std::cout << "\n② v[99999]（不检查）：\n";
#ifdef OOB_DEMO
    std::cout << "   " << v[99999] << "\n";   // 用 -DOOB_DEMO 打开才会真读
#else
    std::cout << "   本例默认不真的越界读（否则 ASan 会 abort）。\n"
              << "   要演示请用：clang++ -DOOB_DEMO -fsanitize=address ... \n"
              << "   或直接看 examples/3.3.3-1-index-and-erase.cpp 的 ASan 说明。\n";
#endif

    // ③ 越界写：比读更危险，静默踩坏邻居
    std::cout << "\n③ 越界写才是真正的灾难：\n";
    std::vector<int> a(4, 1), b(4, 2);
    std::cout << "   a.data()=" << static_cast<const void*>(a.data())
              << "  b.data()=" << static_cast<const void*>(b.data()) << "\n";
#ifdef OOB_WRITE_DEMO
    a[4] = 999;                               // 越界写：可能正好写进 b
    std::cout << "   写入 a[4]=999 后 b[0]=" << b[0] << "  ← 邻居被改了！\n";
#else
    std::cout << "   （用 -DOOB_WRITE_DEMO 打开越界写演示：a[4]=999 可能把 b[0] 改成 999）\n"
              << "   这正是 HFT 里\"偶发错价\"的典型来源：不崩、不报、静默污染。\n";
#endif

    std::cout << "\n④ 工程对策：\n"
              << "   · 下标来自协议字段/外部输入 → 用 at() 或 [] 前显式 assert\n"
              << "   · CI 里开 libc++ hardening：-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG\n"
              << "     （让 operator[] 也做检查，线上关掉）\n"
              << "   · 历史回放数据用 ASan 跑一遍\n";
    return 0;
}
