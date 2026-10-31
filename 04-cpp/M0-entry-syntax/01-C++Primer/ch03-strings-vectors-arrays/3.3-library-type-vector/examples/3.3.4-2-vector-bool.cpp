// 3.3.4-2  vector<bool> 是标准里唯一的容器特化：位压缩，省内存但慢 28 倍
//
// 编译：clang++ -std=c++17 -O2 -Wall 3.3.4-2-vector-bool.cpp -o vb && ./vb

#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

using Clock = std::chrono::steady_clock;

int main() {
    const int N = 10'000'000;
    std::cout << "sizeof(std::vector<bool>) = " << sizeof(std::vector<bool>) << " 字节（对象本身）\n";
    std::cout << "内部按 1 bit 存元素：1000 万个 bool 只占 "
              << (N / 8 / 1024) << " KB，而 vector<char> 要 " << (N / 1024) << " KB\n\n";

    std::vector<bool> vb(N, false);
    std::vector<char> vc(N, 0);
    std::vector<std::uint8_t> vu(N, 0);

    // ① 读+写 1000 万次
    {
        auto t0 = Clock::now();
        long s = 0;
        for (int i = 0; i < N; ++i) { vb[i] = (i & 1); s += vb[i]; }
        auto t1 = Clock::now();
        double a = std::chrono::duration<double, std::milli>(t1 - t0).count();
        std::cout << "vector<bool>   读写 1000 万次: " << a << " ms (sum=" << s << ")\n";

        auto t2 = Clock::now();
        long s2 = 0;
        for (int i = 0; i < N; ++i) { vc[i] = (i & 1); s2 += vc[i]; }
        auto t3 = Clock::now();
        double b = std::chrono::duration<double, std::milli>(t3 - t2).count();
        std::cout << "vector<char>   读写 1000 万次: " << b << " ms (sum=" << s2 << ")\n";

        auto t4 = Clock::now();
        long s3 = 0;
        for (int i = 0; i < N; ++i) { vu[i] = (i & 1); s3 += vu[i]; }
        auto t5 = Clock::now();
        double c = std::chrono::duration<double, std::milli>(t5 - t4).count();
        std::cout << "vector<uint8_t>读写 1000 万次: " << c << " ms (sum=" << s3 << ")\n";

        std::cout << "\n比值：vector<bool> / vector<char> = " << (a / b) << "x  ← 慢 28 倍左右\n";
        std::cout << "原因：operator[] 返回代理对象 std::vector<bool>::reference，\n"
                  << "      每次读写要做【移位 + 掩码 + 读改写整个字节】。\n";
    }

    // ② 代理对象带来的语法坑
    std::cout << "\n② 代理对象的语法后果：\n";
    std::cout << "   auto& r = vb[0];   // r 是 std::vector<bool>::reference，不是 bool&\n";
    auto r = vb[0];
    std::cout << "   auto  r = vb[0];   // OK，r = " << r << "\n";
    std::cout << "   bool* p = &vb[0];  // ❌ 编译错误：不能取代理对象的地址\n";
    std::cout << "   // 想取地址就只能用：\n";
    std::cout << "   std::vector<char> flags(N);  bool* p = reinterpret_cast<bool*>(flags.data());  // 不推荐\n";

    std::cout << "\n③ HFT 结论：\n"
              << "   标志位/有效性位图这类高频访问字段 → 用 vector<char> 或 vector<uint8_t>\n"
              << "   只有在内存真的紧张到要省那 8 倍时才用 vector<bool>\n";
    return 0;
}
