#pragma once

/*
 * feed_handler.hpp — 行情接收管线（Phase 3 核心）。
 *
 *   UDP 数据报 ─→ 解帧(codec) ─→ 序列号状态机(Sequencer)
 *       ├─ Accept    → 交付 Sink + 冲刷乱序缓冲
 *       ├─ Duplicate → 丢弃（计数）
 *       └─ Gap       → 乱序缓冲收容；超窗/断档持续 → RECOVERING：发重传请求
 *
 * 恢复（重传通道灌入 on_retransmit_*）：
 *   - 恢复是权威源：灌入前清空乱序缓冲（里面的旧乱序与恢复数据重复）；
 *   - 恢复数据必须从 expect 开始连续，否则对端状态与我们不一致（告警）；
 *   - 恢复可以是业务消息（逐条补）或 GapFill（"这段没有业务消息"）。
 *
 * 「回放 = 实盘同码路径」：on_datagram 是**唯一入口**——实盘从 socket recv
 * 灌进来，回放从文件读出来灌进来，下游（解帧/排序/恢复/交付）是同一份代码。
 *
 * 重传通道（TCP）不在本阶段：恢复请求通过回调暴露，实盘按 venue 接。
 */

#include <cstdint>
#include <functional>
#include <utility>

#include "common/compiler.hpp"
#include "common/types.hpp"
#include "feed/reorder_buffer.hpp"
#include "protocol/codec.hpp"
#include "protocol/messages.hpp"
#include "protocol/sequencer.hpp"

namespace hft::feed {

struct FeedStats {
    std::uint64_t datagrams = 0;
    std::uint64_t messages = 0;
    std::uint64_t delivered = 0;
    std::uint64_t duplicates = 0;         // 重传/重复包
    std::uint64_t reordered = 0;          // 乱序缓冲接住后又交付的
    std::uint64_t gaps = 0;               // 触发恢复请求的次数
    std::uint64_t retransmit_requests = 0;
    std::uint64_t recovered_msgs = 0;     // 恢复通道交付的业务消息
    std::uint64_t gap_fills = 0;          // GapFill 推进次数
    std::uint64_t slot_conflicts = 0;     // 乱序缓冲槽位冲突（协议异常）
    std::uint64_t malformed = 0;          // 解码失败的报文
    std::uint64_t resync_mismatch = 0;    // 恢复数据与 expect 对不上（告警）
};

enum class FeedState : std::uint8_t {
    Normal,      // 正常接收
    Recovering,  // 断档：已发重传请求，等恢复数据（新到组播消息继续入乱序缓冲）
};

/* Sink 概念：void operator()(const proto::Message&) —— 交付一条已排序消息 */
template <typename Sink>
class FeedHandler {
public:
    /* retransmit_cb(gap_start, last_seen_seq)：触发重传请求时回调。
     * 测试里直接灌 on_retransmit_message/on_retransmit_gap_fill；
     * 实盘在这里接 TCP 重传客户端。 */
    using RetransmitCb = std::function<void(SeqNum gap_start, SeqNum last_seen)>;

    explicit FeedHandler(Sink sink, RetransmitCb retransmit_cb = {})
        : sink_(std::forward<Sink>(sink)), retransmit_cb_(std::move(retransmit_cb)) {}

    // ---------- 唯一入口：一个 UDP 数据报 ----------
    void on_datagram(const std::uint8_t* data, std::size_t len) noexcept {
        ++stats_.datagrams;
        std::size_t pos = 0;
        while (pos < len) {
            proto::Message m;
            const std::size_t used = proto::decode(data + pos, len - pos, m);
            if (HFT_UNLIKELY(used == 0)) {
                if (pos + 1 < len) ++stats_.malformed;   // 半截畸形报文
                break;   // 游标停住：剩余部分按畸形丢弃（协议无自愈义务）
            }
            pos += used;
            ++stats_.messages;
            on_message(m);
        }
    }

    // ---------- 恢复通道 ----------
    /* 重传服务器补来一条业务消息。必须从 expect 开始（恢复是权威的）。 */
    bool on_retransmit_message(const proto::Message& m) noexcept {
        if (state_ != FeedState::Recovering) return true;  // 非恢复期：理论上不该来，宽容忽略
        if (HFT_UNLIKELY(m.hdr.seq != seq_.expect())) {
            ++stats_.resync_mismatch;
            return false;
        }
        begin_recovery_if_needed_();  // no-op 防误用
        (void)seq_.on_message(m.hdr.seq);
        deliver_(m);
        ++stats_.recovered_msgs;
        return true;
    }

    /* 重传服务器告知 [from, to] 区间没有业务消息（GapFill）。 */
    bool on_retransmit_gap_fill(SeqNum from, SeqNum to) noexcept {
        if (HFT_UNLIKELY(!seq_.on_gap_fill(from, to))) {
            ++stats_.resync_mismatch;
            return false;
        }
        ++stats_.gap_fills;
        return true;
    }

    /* 恢复完成：drain 恢复期间积在乱序缓冲里的连续前缀，回 Normal。 */
    void finish_recovery() noexcept {
        flush_buffer_();
        state_ = FeedState::Normal;
    }

    [[nodiscard]] const FeedStats& stats() const noexcept { return stats_; }
    [[nodiscard]] FeedState state() const noexcept { return state_; }
    [[nodiscard]] SeqNum expect() const noexcept { return seq_.expect(); }

private:
    void on_message(const proto::Message& m) noexcept {
        if (m.type == proto::MsgType::GapFill) {
            if (seq_.on_gap_fill(m.gap_fill.from, m.gap_fill.to)) ++stats_.gap_fills;
            flush_buffer_();
            return;
        }
        // 心跳/会话消息不进序列号判定？——ITCH 惯例全部进（seq 单调覆盖全类型）。
        switch (seq_.on_message(m.hdr.seq)) {
        case proto::SeqVerdict::Accept:
            deliver_(m);
            flush_buffer_();
            break;
        case proto::SeqVerdict::Duplicate:
            ++stats_.duplicates;
            break;
        case proto::SeqVerdict::Gap:
            handle_gap_(m);
            break;
        }
    }

    void deliver_(const proto::Message& m) noexcept {
        sink_(m);
        ++stats_.delivered;
    }

    void flush_buffer_() noexcept {
        SeqNum e = seq_.expect();
        const std::size_t n = buf_.drain(e, [this](const proto::Message& m) { deliver_(m); });
        stats_.reordered += n;
        seq_.resync(e);
    }

    /* 触发恢复的缓冲水位：小乱序靠缓冲理顺，超水位才认为真丢包。
     * 实盘按 feed 的乱序率调（组播主备双路时常见值 32~256）。 */
    static constexpr std::size_t kGapTrigger = 32;

    void handle_gap_(const proto::Message& m) noexcept {
        last_seen_seq_ = m.hdr.seq;
        switch (buf_.put(seq_.expect(), m.hdr.seq, m)) {
        case PutResult::Stored:
            // 小乱序收容，等后续包填上（drain 自动理顺）；
            // 缓冲水位超阈值 = 缺口大概率是真的丢了 → 触发恢复
            if (buf_.used() >= kGapTrigger) {
                enter_recovery_();
            }
            break;
        case PutResult::TooOld:
            ++stats_.duplicates;
            break;
        case PutResult::OutOfWindow:
            enter_recovery_();
            break;
        case PutResult::SlotConflict:
            ++stats_.slot_conflicts;
            enter_recovery_();
            break;
        }
    }

    void enter_recovery_() noexcept {
        if (state_ == FeedState::Recovering) return;  // 不重复发请求
        state_ = FeedState::Recovering;
        gap_start_ = seq_.expect();
        ++stats_.gaps;
        ++stats_.retransmit_requests;
        if (retransmit_cb_) {
            retransmit_cb_(gap_start_, last_seen_seq_);
        }
    }

    void begin_recovery_if_needed_() noexcept {}

    Sink sink_;
    RetransmitCb retransmit_cb_;
    proto::Sequencer seq_;
    ReorderBuffer<4096> buf_;
    FeedStats stats_;
    FeedState state_ = FeedState::Normal;
    SeqNum gap_start_ = 0;
    SeqNum last_seen_seq_ = 0;
};

} // namespace hft::feed
