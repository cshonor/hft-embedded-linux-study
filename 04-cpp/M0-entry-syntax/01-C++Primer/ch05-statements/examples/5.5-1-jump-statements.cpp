// 5.5 break / continue / goto 的行为与代价
//
// 编译：clang++ -std=c++17 -O2 5.5-1-jump-statements.cpp -o jump && ./jump
#include <chrono>
#include <cstdio>

// ① 经典输出题：break 只跳出最内层
static void quiz1() {
    printf("① 嵌套循环里的 break：");
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (i + j == 2) break;
            printf("%d%d ", i, j);
        }
    }
    printf("\n");
}

// ② while 里的 continue 会跳过 ++i → 死循环（这里加保险丝）
static void quiz2() {
    int i = 0, guard = 0;
    while (i < 5) {
        if (++guard > 20) break;    // 保险丝必须放在 continue 之前
        if (i % 2 == 0) continue;   // ← i 永远是 0，++i 被跳过
        ++i;
    }
    printf("② while + continue：i=%d（guard 跑了 %d 次 → 实际是死循环）\n", i, guard);
}

// ③ 跳出多层循环的三种写法
static const int N = 400;
static int g_mat[N][N];

static int find_goto(int target) {              // goto：一次跳出两层
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            if (g_mat[i][j] == target) goto found;
    return -1;
found:
    return 0;
}
static int find_flag(int target) {              // 标志位：每层都要判断
    bool hit = false;
    for (int i = 0; i < N && !hit; ++i)
        for (int j = 0; j < N; ++j)
            if (g_mat[i][j] == target) { hit = true; break; }
    return hit ? 0 : -1;
}
static int find_return(int target) {            // 抽成函数用 return
    struct H {
        static int at(int t) {
            for (int i = 0; i < N; ++i)
                for (int j = 0; j < N; ++j)
                    if (g_mat[i][j] == t) return 0;
            return -1;
        }
    };
    return H::at(target);
}

template <class F>
static void bench(const char* name, F fn, int rounds) {
    auto t0 = std::chrono::steady_clock::now();
    long sink = 0;
    for (int r = 0; r < rounds; ++r) sink += fn(g_mat[100][100]);  // 命中在 (100,100)
    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count() / rounds;
    printf("  %-16s %8.3f ms/轮  (sink=%ld)\n", name, ms, sink);
}

int main() {
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j) g_mat[i][j] = i * N + j;

    quiz1();
    quiz2();

    printf("③ 跳出多层循环（400×400，命中在 (100,100)，20 轮，-O2）\n");
    bench("goto", find_goto, 20);
    bench("标志位", find_flag, 20);
    bench("return(抽函数)", find_return, 20);
    return 0;
}
