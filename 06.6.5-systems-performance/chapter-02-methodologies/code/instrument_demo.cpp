// instrument_demo.cpp — P0 分层埋点最小实现
// 出自: notes/section-2.5-性能分析方法论.md 块3（atomic counter 埋点）
//
// 要点：
//   - 热路径上只做 fetch_add(relaxed)，几十 cycles，不打日志、不格式化
//   - 指标计算（delta/Δt、分位数）全部挪到旁路线程周期性做
//   - ticks/s = delta(counter) / Δt —— 这就是喂 Grafana panel 的那个数
//
// 编译: g++ -g -O2 -Wall -pthread -o instrument_demo instrument_demo.cpp
// 运行: ./instrument_demo（3 秒后自动退出）

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>

// ---------- P0 埋点：热路径上唯一的开销 ----------
std::atomic<uint64_t> g_parse_entry{0};    // L2 解析入口 counter（ticks/s 的真相源）
std::atomic<uint64_t> g_reject{0};         // L3 策略/报单 reject counter

// 模拟热路径：收到一个行情 buffer
inline void on_market_buffer(const char* /*buf*/, size_t /*len*/) {
    g_parse_entry.fetch_add(1, std::memory_order_relaxed);
    // ... decode（此处用空循环模拟几十 ns 的解析成本）...
    for (volatile int i = 0; i < 10; i++) {}
}

int main() {
    std::atomic<bool> stop{false};

    // 旁路线程：周期性读差 → ticks/s（不在热路径上）
    std::thread reporter([&] {
        uint64_t prev = g_parse_entry.load(std::memory_order_relaxed);
        auto prev_t = std::chrono::steady_clock::now();
        for (int round = 0; round < 3; round++) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            uint64_t now = g_parse_entry.load(std::memory_order_relaxed);
            auto now_t = std::chrono::steady_clock::now();
            double dt = std::chrono::duration<double>(now_t - prev_t).count();
            std::printf("[reporter] ticks/s = %.0f  (delta=%lu, dt=%.2fs)  reject=%lu\n",
                        (now - prev) / dt, now - prev, dt,
                        g_reject.load(std::memory_order_relaxed));
            prev = now;
            prev_t = now_t;
        }
        stop.store(true, std::memory_order_relaxed);
    });

    // 热路径主循环：模拟行情驱动
    char buf[256] = {};
    while (!stop.load(std::memory_order_relaxed)) {
        on_market_buffer(buf, sizeof(buf));
    }

    reporter.join();
    std::printf("total ticks = %lu\n", g_parse_entry.load());
    return 0;
}
