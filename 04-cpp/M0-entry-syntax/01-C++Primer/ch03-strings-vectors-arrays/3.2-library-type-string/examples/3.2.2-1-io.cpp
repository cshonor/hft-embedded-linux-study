// 3.2.2 —— 读写操作实测：>> 与 getline 的分隔规则、混用陷阱、解析性能
// 编译：clang++ -std=c++17 -O2 examples/3.2.2-1-io.cpp -o io && ./io
#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

static std::string make_lines(int n) {
    std::string s;
    s.reserve(n * 24);
    for (int i = 0; i < n; ++i) {
        s += "OID"; s += std::to_string(i);
        s += " 3750."; s += std::to_string(i % 100);
        s += " 10\n";
    }
    return s;
}

int main() {
    printf("① >> 跳过所有空白，getline 只认换行\n");
    {
        std::istringstream in("   hello   world\n");
        std::string w1, w2;
        in >> w1 >> w2;
        printf("  >>      : w1=\"%s\" w2=\"%s\"\n", w1.c_str(), w2.c_str());
    }
    {
        std::istringstream in("   hello   world\n");
        std::string line;
        std::getline(in, line);
        printf("  getline : line=\"%s\"   ← 空白全保留\n", line.c_str());
    }

    printf("② 混用陷阱：>> 留下的 \\n 让 getline 读到空行\n");
    {
        std::istringstream in("42\nremark text\n");
        int n = 0; std::string line;
        in >> n;
        std::getline(in, line);
        printf("  不处理      : n=%d, line=\"%s\"   ← 空！\n", n, line.c_str());
    }
    {
        std::istringstream in("42\nremark text\n");
        int n = 0; std::string line;
        in >> n;
        in.ignore();
        std::getline(in, line);
        printf("  ignore()    : n=%d, line=\"%s\"\n", n, line.c_str());
    }
    {
        std::istringstream in("42\nremark text\n");
        int n = 0; std::string line;
        in >> n >> std::ws;              // std::ws 吃掉全部连续空白（含多行）
        std::getline(in, line);
        printf("  >> std::ws  : n=%d, line=\"%s\"   ← 推荐\n", n, line.c_str());
    }

    printf("③ 解析 100 万行订单流：三种读法\n");
    const int N = 1'000'000;
    std::string data = make_lines(N);
    {
        auto t0 = std::chrono::steady_clock::now();
        std::istringstream in(data);
        std::string oid; double px; int q; long sum = 0;
        while (in >> oid >> px >> q) sum += q;
        auto t1 = std::chrono::steady_clock::now();
        printf("  istringstream >> : %6.1f ms (sum=%ld)\n",
               std::chrono::duration<double, std::milli>(t1 - t0).count(), sum);
    }
    {
        auto t0 = std::chrono::steady_clock::now();
        std::istringstream in(data);
        std::string line; long sum = 0;
        while (std::getline(in, line)) sum += (int)(line.back() - '0');
        auto t1 = std::chrono::steady_clock::now();
        printf("  getline + 手写切 : %6.1f ms (sum=%ld)\n",
               std::chrono::duration<double, std::milli>(t1 - t0).count(), sum);
    }
    {
        const char* fn = "/tmp/hft_lines.txt";
        { std::ofstream o(fn); o << data; }
        auto t0 = std::chrono::steady_clock::now();
        FILE* f = fopen(fn, "r");
        char oid[32]; double px; int q; long sum = 0;
        while (fscanf(f, "%31s %lf %d", oid, &px, &q) == 3) sum += q;
        fclose(f);
        auto t1 = std::chrono::steady_clock::now();
        printf("  fscanf           : %6.1f ms (sum=%ld)\n",
               std::chrono::duration<double, std::milli>(t1 - t0).count(), sum);
    }
    printf("  ※ >> 比 getline+手写慢约 7 倍：每次 >> 都要构造 sentry、查 locale、处理流状态。\n"
           "    回放百万级行情时用 getline + 手写切分（或直接 mmap + 指针扫）。\n");

    printf("④ sync_with_stdio：cin 默认与 C stdio 同步\n");
    printf("  main 开头写 std::ios::sync_with_stdio(false); std::cin.tie(nullptr);\n"
           "  可让 cin 接近 scanf 的速度（默认同步下 cin 通常比 scanf 慢数倍）\n");
    return 0;
}
