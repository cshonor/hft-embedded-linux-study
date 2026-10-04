#pragma once

/*
 * compiler.hpp — 编译器/机器相关的生产级开关。
 *
 * 生产 HFT 代码里这些宏不是装饰：
 *   - HFT_LIKELY/HFT_UNLIKELY 喂给分支预测器，热路径少跳一次就是几纳秒；
 *   - kCacheLine 是所有对齐/伪共享防护的唯一来源，换架构（如 ARM 部分核心 128B）只改这里；
 *   - HFT_FORCE_INLINE 用于 codec / ring 这类必须内联的函数，禁止依赖编译器心情。
 *
 * 注意：__builtin_expect 只对 gcc/clang 有效，MSVC 需要回退——本项目目标平台是
 * Linux + gcc/clang，Mac 仅做编译期验证，不为 MSVC 写兼容层。
 */

#include <cstddef>

#if defined(__GNUC__) || defined(__clang__)
#  define HFT_LIKELY(x)   __builtin_expect(!!(x), 1)
#  define HFT_UNLIKELY(x) __builtin_expect(!!(x), 0)
#  define HFT_FORCE_INLINE inline __attribute__((always_inline))
#  define HFT_NOINLINE __attribute__((noinline))
#  define HFT_HOT __attribute__((hot))
#  define HFT_COLD __attribute__((cold))
#else
#  define HFT_LIKELY(x)   (x)
#  define HFT_UNLIKELY(x) (x)
#  define HFT_FORCE_INLINE inline
#  define HFT_NOINLINE
#  define HFT_HOT
#  define HFT_COLD
#endif

namespace hft {

// x86-64 与 Apple Silicon / Cortex-A76（Pi 5）都是 64B cache line。
// Cortex-A720/X 系有 128B 情形，跨平台部署时按 -DHFT_CACHELINE=128 覆盖。
#ifndef HFT_CACHELINE
inline constexpr std::size_t kCacheLine = 64;
#else
inline constexpr std::size_t kCacheLine = HFT_CACHELINE;
#endif

// 防止伪共享的通用手法：生产/消费计数器各占一条 cache line。
#define HFT_ALIGN_CACHELINE alignas(::hft::kCacheLine)

} // namespace hft
