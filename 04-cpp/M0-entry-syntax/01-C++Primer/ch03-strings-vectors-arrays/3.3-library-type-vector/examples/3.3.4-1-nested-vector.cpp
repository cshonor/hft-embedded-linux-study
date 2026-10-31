// 3.3.4 —— 元素类型与嵌套容器：内存布局、分配次数、vector<bool> 特化
// 编译：clang++ -std=c++17 -O2 examples/3.3.4-1-nested-vector.cpp -o nested && ./nested
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// 让缓冲区逃逸，阻止 clang 把 malloc/free 配对整段消掉
// （不这么做的话扁平那组会测出 0.00008 ms 的假数据）
static void* volatile escape = nullptr;
static volatile long vsink = 0;
static long alloc_count = 0;
void* operator new(std::size_t n) { ++alloc_count; return std::malloc(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

int main() {
    printf("① 嵌套 vector 的内存布局：外层连续，内层各自分配\n");
    std::vector<std::vector<int>> mat;
    for (int i = 0; i < 4; ++i) mat.emplace_back(std::vector<int>(4, i));
    printf("  外层对象：&mat[0]=%p  &mat[1]=%p（相差 %ld 字节 = 一个 vector 对象大小）\n",
           (void*)&mat[0], (void*)&mat[1], (long)((char*)&mat[1] - (char*)&mat[0]));
    for (int i = 0; i < 4; ++i)
        printf("  mat[%d].data() = %p\n", i, (void*)mat[i].data());
    printf("  → 四行地址互不相邻：每行一次独立的 operator new\n");

    printf("② 构造成本：512×512 两种写法\n");
    const int S = 512, ROUNDS = 50;
    for (int rep = 0; rep < 3; ++rep) {
        alloc_count = 0;
        auto t0 = std::chrono::steady_clock::now();
        for (int r = 0; r < ROUNDS; ++r) {
            std::vector<std::vector<int>> m(S, std::vector<int>(S));
            for (int i = 0; i < S; ++i) m[i][i] = i;   // 真实写入
            escape = m[0].data();
            vsink  = m[S-1][S-1];
        }
        auto t1 = std::chrono::steady_clock::now();
        long na = alloc_count;

        alloc_count = 0;
        auto t2 = std::chrono::steady_clock::now();
        for (int r = 0; r < ROUNDS; ++r) {
            std::vector<int> f(S * S);
            for (int i = 0; i < S; ++i) f[i * S + i] = i;   // 同样的写入
            escape = f.data();
            vsink  = f[S * S - 1];
        }
        auto t3 = std::chrono::steady_clock::now();
        long nb = alloc_count;

        double a = std::chrono::duration<double, std::milli>(t1 - t0).count();
        double b = std::chrono::duration<double, std::milli>(t3 - t2).count();
        printf("  rep%d: 嵌套 %6.2f ms (%ld 次分配/轮)   扁平 %6.2f ms (%ld 次/轮)   比值 %.1fx\n",
               rep, a, na / ROUNDS, b, nb / ROUNDS, a / b);
    }
    printf("  ※ 差距在【构造】：嵌套 514 次 malloc vs 扁平 1 次，约 7 倍。\n");

    printf("③ 顺序遍历：两者性能相同（都被向量化）\n");
    std::vector<std::vector<int>> m2(S, std::vector<int>(S, 1));
    std::vector<int> f2(S * S, 1);
    for (int rep = 0; rep < 2; ++rep) {
        auto t0 = std::chrono::steady_clock::now();
        long s = 0;
        for (int r = 0; r < 20; ++r)
            for (int i = 0; i < S; ++i) for (int j = 0; j < S; ++j) s += m2[i][j];
        auto t1 = std::chrono::steady_clock::now();

        auto t2 = std::chrono::steady_clock::now();
        long s2 = 0;
        for (int r = 0; r < 20; ++r)
            for (int i = 0; i < S; ++i) for (int j = 0; j < S; ++j) s2 += f2[i * S + j];
        auto t3 = std::chrono::steady_clock::now();
        printf("  run %d: 嵌套 %5.1f ms  扁平 %5.1f ms  (sum=%ld/%ld)\n", rep,
               std::chrono::duration<double, std::milli>(t1 - t0).count(),
               std::chrono::duration<double, std::milli>(t3 - t2).count(), s, s2);
    }
    printf("  ※ 修正一个常见说法：顺序遍历时嵌套并不比扁平慢（实测都是 0.6 ms）。\n"
           "     真正的差距在【构造/分配次数】和【随机访问】上，不在顺序遍历。\n");

    printf("④ vector<bool> 是特化，不是容器\n");
    std::vector<bool> vb{true, false, true};
    printf("  sizeof(vector<bool>) = %zu，sizeof(bool) = %zu\n",
           sizeof(std::vector<bool>), sizeof(bool));
    printf("  vb[0] = %d（返回的是代理对象 __bit_reference，不是 bool&）\n", (int)vb[0]);
    printf("  → 不能取 &vb[0]，不能传给 bool& 形参；要真容器用 std::vector<char>\n");

    printf("⑤ sizeof 对照（决定一个 cache line 装几个）\n");
    struct Tick { double bid, ask; long qty, ts; };
    printf("  int=%zu  double=%zu  Tick=%zu  string=%zu  vector<int>=%zu  unique_ptr=%zu\n",
           sizeof(int), sizeof(double), sizeof(Tick), sizeof(std::string),
           sizeof(std::vector<int>), sizeof(std::unique_ptr<int>));
    printf("  ※ string / vector<int> 本身只有 24 字节，数据在另一块堆 —— 遍历时多一跳\n");

    printf("⑥ 指针元素：10 万个元素 = 10 万次堆分配\n");
    {
        alloc_count = 0;
        auto t0 = std::chrono::steady_clock::now();
        std::vector<std::unique_ptr<Tick>> v; v.reserve(100000);
        for (int i = 0; i < 100000; ++i) v.push_back(std::make_unique<Tick>());
        auto t1 = std::chrono::steady_clock::now();
        printf("  vector<unique_ptr<Tick>> : %6.2f ms  分配 %ld 次\n",
               std::chrono::duration<double, std::milli>(t1 - t0).count(), alloc_count);

        alloc_count = 0;
        auto t2 = std::chrono::steady_clock::now();
        std::vector<Tick> w; w.reserve(100000);
        for (int i = 0; i < 100000; ++i) w.push_back(Tick{});
        auto t3 = std::chrono::steady_clock::now();
        printf("  vector<Tick>             : %6.2f ms  分配 %ld 次\n",
               std::chrono::duration<double, std::milli>(t3 - t2).count(), alloc_count);
        printf("  ※ 实测 5~11 倍（多次运行波动大），但分配次数稳定是 100001 : 1。\n"
               "     代价换来的是：元素不可拷贝 / 多态 / 扩容时对象地址不变\n");
    }

    printf("⑦ 不可拷贝也不可移动的类型进不了 vector\n");
    printf("  std::vector<std::mutex> mv;  // 编译错误：mutex 不可拷贝也不可移动\n");
    std::vector<std::unique_ptr<std::mutex>> mv2;
    mv2.push_back(std::make_unique<std::mutex>());
    printf("  std::vector<unique_ptr<mutex>> → OK，实测 size=%zu（搬的是指针）\n", mv2.size());

    printf("⑧ vector<T*> 不自动管理\n");
    printf("  vector 只管指针本身的内存，不管指针指向的对象 —— 析构不会 delete 元素。\n"
           "  要自动管理用 vector<unique_ptr<T>>；要值语义直接用 vector<T>。\n");
    return 0;
}
