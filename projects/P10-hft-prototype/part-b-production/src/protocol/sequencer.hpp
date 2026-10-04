#pragma once

/*
 * sequencer.hpp — 流序列号状态机。
 *
 * 行情组播是 UDP，丢包是常态，不是异常。生产语义：
 *   - 每条消息带单调 seq（从 1 开始）；
 *   - 收到 expect 之外的消息：seq > expect = gap（去重传恢复）；seq < expect = 重复/重传包（丢弃）；
 *   - GapFill 消息：重传服务器说"这段没有业务消息"，直接推进 expect，不进 book。
 *
 * Phase 3 会在这上面加乱序缓冲 + 重传请求通道；本状态机是它的心脏，
 * 所以单独成文件并配自测。
 */

#include <cstdint>

#include "common/compiler.hpp"
#include "common/types.hpp"

namespace hft::proto {

enum class SeqVerdict : std::uint8_t {
    Accept,    // 正好是 expect：交付上层，expect 前进
    Duplicate, // seq < expect：重传/重复，静默丢弃
    Gap,       // seq > expect：丢了 [expect, seq-1]，需恢复
};

class Sequencer {
public:
    void reset(SeqNum first = 1) noexcept { expect_ = first; }

    [[nodiscard]] SeqNum expect() const noexcept { return expect_; }

    // 判一条新到的业务消息。
    [[nodiscard]] HFT_FORCE_INLINE SeqVerdict on_message(SeqNum seq) noexcept {
        if (HFT_LIKELY(seq == expect_)) {
            ++expect_;
            return SeqVerdict::Accept;
        }
        return (seq < expect_) ? SeqVerdict::Duplicate : SeqVerdict::Gap;
    }

    // 判一条 GapFill：合法区间必须覆盖 expect，否则对端状态与我们不一致（生产要告警）。
    [[nodiscard]] HFT_FORCE_INLINE bool on_gap_fill(SeqNum from, SeqNum to) noexcept {
        if (HFT_UNLIKELY(from != expect_ || to < from)) return false;
        expect_ = to + 1;
        return true;
    }

    // 重传恢复完成：直接对齐到新 expect（Phase 3 用）。
    void resync(SeqNum next) noexcept { expect_ = next; }

private:
    SeqNum expect_ = 1;
};

} // namespace hft::proto
