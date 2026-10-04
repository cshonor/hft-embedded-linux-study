#pragma once

/*
 * level_bitmap.hpp — 价位占用两级位图。
 *
 * 为什么不用红黑树（std::map）做价位索引：
 *   - 树节点要堆分配（违反热路径零分配）或内存池+指针追逐（cache miss）；
 *   - 有价证券的 tick 域是有界的（涨跌停/价格带），数组+位图是 O(1) 且
 *     访问局部性好一个数量级——这是生产订单簿的主流做法。
 *
 * 结构：NumLevels 个价位 → 每 64 价位一个 u64 数据字 → 每 64 个数据字一个摘要位。
 * set/clear O(1)；find_first/find_last 最多扫 NumLevels/4096 个摘要字
 * （65536 价位 = 16 字）+ 1 个数据字，分支高度可预测。
 */

#include <cstddef>
#include <cstdint>

#include "common/compiler.hpp"

namespace hft {

template <std::size_t NumLevels>
class LevelBitmap {
    static constexpr std::size_t kWords   = (NumLevels + 63) / 64;
    static constexpr std::size_t kSummary = (kWords + 63) / 64;

public:
    HFT_FORCE_INLINE void set(std::size_t i) noexcept {
        const std::size_t w = i >> 6;
        words_[w] |= (1ull << (i & 63));
        summary_[w >> 6] |= (1ull << (w & 63));
    }

    HFT_FORCE_INLINE void clear(std::size_t i) noexcept {
        const std::size_t w = i >> 6;
        words_[w] &= ~(1ull << (i & 63));
        if (words_[w] == 0) {
            summary_[w >> 6] &= ~(1ull << (w & 63));
        }
    }

    // 最低非空价位下标；全空返回 -1。
    [[nodiscard]] int find_first() const noexcept {
        for (std::size_t s = 0; s < kSummary; ++s) {
            const std::uint64_t sw = summary_[s];
            if (sw == 0) continue;
            // 摘要位定位到数据字，再 ctz 到位。摘要不空 ⇒ 该 64 字组内必有非空字。
            std::size_t w = (s << 6) + static_cast<std::size_t>(__builtin_ctzll(sw));
            // 该摘要位对应的数据字可能本身为空（末组越界保护），向后找。
            for (; w < kWords && words_[w] == 0; ++w) {}
            if (w < kWords) {
                return static_cast<int>((w << 6) +
                    static_cast<std::size_t>(__builtin_ctzll(words_[w])));
            }
        }
        return -1;
    }

    // 最高非空价位下标；全空返回 -1。
    [[nodiscard]] int find_last() const noexcept {
        for (std::size_t s = kSummary; s-- > 0;) {
            const std::uint64_t sw = summary_[s];
            if (sw == 0) continue;
            std::size_t w = (s << 6) +
                static_cast<std::size_t>(63 - __builtin_clzll(sw));
            for (;;) {
                if (words_[w] != 0) {
                    return static_cast<int>((w << 6) +
                        static_cast<std::size_t>(63 - __builtin_clzll(words_[w])));
                }
                if (w == (s << 6)) break;
                --w;
            }
        }
        return -1;
    }

    // 严格小于 i 的最高置位；无返回 -1。按 64 价位/字跳跃，供最优价重扫。
    [[nodiscard]] int find_last_below(std::size_t i) const noexcept {
        if (i == 0) return -1;
        --i;
        std::size_t w = i >> 6;
        std::uint64_t mask = ((i & 63) == 63) ? ~0ull : ((1ull << ((i & 63) + 1)) - 1);
        for (;;) {
            const std::uint64_t x = words_[w] & mask;
            if (x != 0) {
                return static_cast<int>((w << 6) +
                    static_cast<std::size_t>(63 - __builtin_clzll(x)));
            }
            if (w == 0) return -1;
            --w;
            mask = ~0ull;
        }
    }

    // 严格大于 i 的最低置位；无返回 -1。
    [[nodiscard]] int find_first_above(std::size_t i) const noexcept {
        ++i;
        if (i >= NumLevels) return -1;
        std::size_t w = i >> 6;
        std::uint64_t mask = ~0ull << (i & 63);
        for (;;) {
            const std::uint64_t x = words_[w] & mask;
            if (x != 0) {
                return static_cast<int>((w << 6) +
                    static_cast<std::size_t>(__builtin_ctzll(x)));
            }
            ++w;
            if (w >= kWords) return -1;
            mask = ~0ull;
        }
    }

private:
    std::uint64_t words_[kWords]     = {};
    std::uint64_t summary_[kSummary] = {};
};

} // namespace hft
