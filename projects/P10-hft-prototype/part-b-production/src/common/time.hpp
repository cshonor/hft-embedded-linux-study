#pragma once

/*
 * time.hpp — 热路径计时抽象。
 *
 * 生产里有两套时钟，用途不同，不能混：
 *   1) 周期计数器（x86 RDTSC / ARM CNTVCT）：~20-30 周期开销，用于热路径延迟采样；
 *      不直接等于纳秒，需要频率换算（tsc freq / cntfrq_el0）。
 *   2) CLOCK_MONOTONIC 纳秒时钟：vdso 加持下 ~20ns，用于日志/风控窗口等"真时间"。
 *
 * 真正上生产还要 PTP 硬件时间戳做跨机对时——那依赖网卡与交换机，
 * 本项目留接口（TsNs 全局流转），不伪造硬件能力。
 */

#include <cstdint>
#include <ctime>

#include "common/compiler.hpp"

namespace hft {

// 纳秒墙钟（单调）。声明在前，rdcycle 兜底分支要用。
HFT_FORCE_INLINE std::uint64_t now_ns() noexcept {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<std::uint64_t>(ts.tv_sec) * 1'000'000'000ull
         + static_cast<std::uint64_t>(ts.tv_nsec);
}

// 周期计数器：原始 tick，不是纳秒。
HFT_FORCE_INLINE std::uint64_t rdcycle() noexcept {
#if defined(__x86_64__)
    std::uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return (static_cast<std::uint64_t>(hi) << 32) | lo;
#elif defined(__aarch64__)
    std::uint64_t v;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(v));
    return v;
#else
    // 兜底：退化成纳秒时钟（语义变了，只保证单调）。
    return now_ns();
#endif
}

} // namespace hft
