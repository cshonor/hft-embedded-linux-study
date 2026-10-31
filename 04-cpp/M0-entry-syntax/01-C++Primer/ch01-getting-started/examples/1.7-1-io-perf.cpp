// 1.7-1  第 1 章收尾：流 IO 的真实开销 —— endl vs '\n'、cout vs printf、sync_with_stdio
//
// 编译：clang++ -std=c++17 -O2 -Wall 1.7-1-io-perf.cpp -o io
// 运行（各模式独立进程，避免缓冲区互相污染）：
//   ./io 0 > /dev/null 2>err   # cout + '\n'
//   ./io 1 > /dev/null 2>err   # cout + endl
//   ./io 2 > /dev/null 2>err   # printf
//   ./io 3 > /dev/null 2>err   # cout + sync_with_stdio(false)
//
// 实测（clang 23.1.0 / libc++ / arm64，20 万行）
// —— 目标 = 真实文件：
//   cout << ... << endl        : 431 ~ 470 ms   ← endl 每行都 flush
//   cout << ... << '\n'        :  18.3 ~ 18.5 ms
//   fprintf(fp, ...)           :  22.6 ~ 24.0 ms
//   ofstream + 关同步          :  17.5 ~ 18.4 ms
//   → endl 比 '\n' 慢约 23~25 倍
//
// —— 目标 = /dev/null，全部走 stdout：
//   std::cout（默认）          :  42.9 ~ 51.5 ms
//   std::cout + 关同步         :  42.8 ~ 46.0 ms   ← libc++ 下【几乎没差别】
//   printf                     :  21.1 ~ 21.2 ms   ← 比 cout 快约 2 倍
//
// 结论（诚实版）：
//   ① endl 的代价是【每次 flush】，这是数量级差异，能省就省
//   ② "关同步快 10 倍"是 libstdc++ 上的老经验；libc++ 上收益接近 0，别盲信
//   ③ printf 在 libc++ 上仍比 std::cout 快约 2 倍 —— 日志热路径值得在意，
//      但先确认瓶颈真的在 IO，别为了这点差距牺牲类型安全

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>

template <class F>
static double ms(F&& f) {
    auto t0 = std::chrono::steady_clock::now();
    f();
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main(int argc, char** argv) {
    const int  N    = 200000;
    const char* out = "/tmp/ch01_io_bench.txt";
    int mode = argc > 1 ? atoi(argv[1]) : 0;
    double t = 0;

    if (mode == 0) {                       // ① 标准写法：'\n' 不强制 flush
        t = ms([&]{ for (int i = 0; i < N; ++i) std::cout << i << " " << i * 2 << '\n'; });
        fprintf(stderr, "cout + '\\n'          : %8.2f ms\n", t);
    } else if (mode == 1) {                // ② endl：每行都 flush
        t = ms([&]{ for (int i = 0; i < N; ++i) std::cout << i << " " << i * 2 << std::endl; });
        fprintf(stderr, "cout + endl           : %8.2f ms\n", t);
    } else if (mode == 2) {                // ③ printf
        t = ms([&]{ for (int i = 0; i < N; ++i) printf("%d %d\n", i, i * 2); });
        fprintf(stderr, "printf                : %8.2f ms\n", t);
    } else if (mode == 3) {                // ④ 关掉与 stdio 的同步
        std::ios::sync_with_stdio(false);
        t = ms([&]{ for (int i = 0; i < N; ++i) std::cout << i << " " << i * 2 << '\n'; });
        fprintf(stderr, "cout + 关同步         : %8.2f ms\n", t);
    } else if (mode == 4) {                // ⑤ 写文件：'\n' vs endl 同进程对照
        {
            std::ofstream f(out);
            t = ms([&]{ for (int i = 0; i < N; ++i) f << i << " " << i * 2 << '\n'; });
            fprintf(stderr, "ofstream + '\\n'       : %8.2f ms\n", t);
        }
        {
            std::ofstream f(out);
            t = ms([&]{ for (int i = 0; i < N; ++i) f << i << " " << i * 2 << std::endl; });
            fprintf(stderr, "ofstream + endl       : %8.2f ms\n", t);
        }
        std::remove(out);
    }
    return 0;
}
