#include "engine.hpp"
#include "self_test.hpp"

#include <cstdlib>
#include <cstring>
#include <string>

/*
 *   ./hft_demo --self-test     先确认撮合规则
 *   ./hft_demo                 默认 2 万 tick 做市回放
 *   ./hft_demo --jump 0        关掉跳价，对比 PnL（逆向选择）
 *
 * 源码阅读顺序：types.hpp → orderbook.hpp → spsc_ring.hpp
 *              → strategy.hpp → risk.hpp → engine.hpp
 */

static void usage() {
    std::printf("usage:\n");
    std::printf("  hft_demo --self-test\n");
    std::printf("  hft_demo [--ticks N] [--seed N] [--hits P] [--jump N]\n");
}

int main(int argc, char** argv) {
    if (argc >= 2 && std::strcmp(argv[1], "--self-test") == 0) {
        return self_test() == 0 ? 0 : 1;
    }
    if (argc >= 2 && (std::strcmp(argv[1], "-h") == 0 || std::strcmp(argv[1], "--help") == 0)) {
        usage();
        return 0;
    }

    ReplayConfig cfg;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto need = [&](const char* flag) -> const char* {
            if (a == flag) {
                if (i + 1 >= argc) {
                    usage();
                    std::exit(2);
                }
                return argv[++i];
            }
            return nullptr;
        };
        if (const char* v = need("--ticks")) {
            cfg.ticks = std::atoi(v);
        } else if (const char* v = need("--seed")) {
            cfg.seed = static_cast<std::uint32_t>(std::atoi(v));
        } else if (const char* v = need("--hits")) {
            cfg.hit_prob = std::atof(v);
        } else if (const char* v = need("--jump")) {
            cfg.jump_every = std::atoi(v);
        } else {
            std::printf("unknown arg: %s\n", argv[i]);
            usage();
            return 2;
        }
    }

    if (cfg.ticks <= 0) {
        std::printf("ticks must be > 0\n");
        return 2;
    }

    std::printf("P10 part-a demo  ticks=%d  seed=%u  hit_prob=%.2f  jump_every=%d\n",
                cfg.ticks, cfg.seed, cfg.hit_prob, cfg.jump_every);
    std::printf("pipeline: replay --SPSC--> book -> market-maker -> risk -> match -> PnL\n");

    run_demo(cfg);
    return 0;
}
