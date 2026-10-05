#pragma once

/*
 * reorder_buffer.hpp — 乱序缓冲（Phase 3）。
 *
 * 网络乱序是常态（多路径/多队列），小窗口乱序不等于丢包。
 * 语义：
 *   - 窗口 N（2 的幂）：[expect, expect+N) 的消息按 seq 落槽；
 *   - 槽位冲突（同 seq 或窗口回绕覆盖旧槽）= 协议异常，拒绝并计数；
 *   - 超窗（seq >= expect+N）= 大 gap，缓冲不下 → 调用方走重传恢复；
 *   - drain：expect 推进后，连续交付槽位非空的前缀。
 *
 * 存完整 Message（含 union）：payload 平凡可复制，热路径 memcpy 安全
 * （messages.hpp 的设计决定在这里兑现）。
 */

#include <cstdint>
#include <cstring>

#include "common/compiler.hpp"
#include "common/types.hpp"
#include "protocol/messages.hpp"

namespace hft::feed {

enum class PutResult : std::uint8_t {
    Stored,       // 落槽成功
    TooOld,       // seq < expect：重传/重复，调用方按 Duplicate 计
    OutOfWindow,  // seq >= expect + N：大 gap，调用方触发重传恢复
    SlotConflict, // 槽位已被同 seq 或更老 seq 占着：协议异常
};

template <std::size_t N>
class ReorderBuffer {
    static_assert((N & (N - 1)) == 0, "window must be power of 2");

public:
    [[nodiscard]] PutResult put(SeqNum expect, SeqNum seq,
                                const proto::Message& m) noexcept {
        if (seq < expect) return PutResult::TooOld;
        const SeqNum off = seq - expect;
        if (off >= N) return PutResult::OutOfWindow;

        Slot& s = slots_[seq & kMask];
        if (HFT_UNLIKELY(s.occupied && s.seq != seq)) return PutResult::SlotConflict;
        if (HFT_UNLIKELY(s.occupied && s.seq == seq)) return PutResult::SlotConflict;

        s.occupied = true;
        s.seq = seq;
        s.msg = m;               // 平凡可复制，一次赋值
        ++used_;
        return PutResult::Stored;
    }

    /* 连续交付：槽[expect] 非空则取出并前进，直到断档。返回交付条数。 */
    template <typename Sink>  // Sink: void(const proto::Message&)
    std::size_t drain(SeqNum& expect, Sink&& sink) noexcept {
        std::size_t n = 0;
        while (true) {
            Slot& s = slots_[expect & kMask];
            if (!s.occupied || s.seq != expect) break;
            sink(s.msg);
            s.occupied = false;
            --used_;
            ++expect;
            ++n;
        }
        return n;
    }

    /* 恢复完成后的对齐：清空残余（重传恢复是权威源，缓冲里的旧乱序全作废）。 */
    void clear() noexcept {
        std::memset(slots_, 0, sizeof(slots_));
        used_ = 0;
    }

    [[nodiscard]] std::size_t used() const noexcept { return used_; }
    [[nodiscard]] static constexpr std::size_t window() { return N; }

    /* 水位检查：expect 处断档但缓冲非空 = 在等一个可能永远不来的 seq（恢复触发依据）。 */
    [[nodiscard]] bool gap_pending(SeqNum expect) const noexcept {
        return used_ > 0 && !(slots_[expect & kMask].occupied && slots_[expect & kMask].seq == expect);
    }

private:
    struct Slot {
        bool occupied = false;
        SeqNum seq = 0;
        proto::Message msg{};
    };
    static constexpr std::size_t kMask = N - 1;

    Slot slots_[N];
    std::size_t used_ = 0;
};

} // namespace hft::feed
