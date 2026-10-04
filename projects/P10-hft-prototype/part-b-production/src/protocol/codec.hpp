#pragma once

/*
 * codec.hpp — 线协议编解码。
 *
 * 生产约束：
 *   - 零堆分配、零异常、零虚函数：解码是返回 bool + 输出参数；
 *   - 每条 UDP 报文可能装多条消息（组播打包惯例），decode 游标式推进；
 *   - 所有长度先校验再读字节，畸形报文只能产生 "reject"，不能产生 UB；
 *   - 大端读写用显式字节拼装（不依赖 __builtin_bswap 的端序假设，
 *     在小端/大端机上结果一致，Pi(aarch64 LE) 与 x86 行为相同）。
 */

#include <cstddef>
#include <cstdint>

#include "common/compiler.hpp"
#include "protocol/messages.hpp"

namespace hft::proto {

class Writer {
public:
    Writer(std::uint8_t* buf, std::size_t cap) : buf_(buf), cap_(cap) {}

    HFT_FORCE_INLINE bool u8(std::uint8_t v) noexcept {
        if (HFT_UNLIKELY(pos_ + 1 > cap_)) return false;
        buf_[pos_++] = v;
        return true;
    }
    HFT_FORCE_INLINE bool u16(std::uint16_t v) noexcept {
        if (HFT_UNLIKELY(pos_ + 2 > cap_)) return false;
        buf_[pos_++] = static_cast<std::uint8_t>(v >> 8);
        buf_[pos_++] = static_cast<std::uint8_t>(v);
        return true;
    }
    HFT_FORCE_INLINE bool u32(std::uint32_t v) noexcept {
        if (HFT_UNLIKELY(pos_ + 4 > cap_)) return false;
        buf_[pos_++] = static_cast<std::uint8_t>(v >> 24);
        buf_[pos_++] = static_cast<std::uint8_t>(v >> 16);
        buf_[pos_++] = static_cast<std::uint8_t>(v >> 8);
        buf_[pos_++] = static_cast<std::uint8_t>(v);
        return true;
    }
    HFT_FORCE_INLINE bool u64(std::uint64_t v) noexcept {
        if (HFT_UNLIKELY(pos_ + 8 > cap_)) return false;
        for (int i = 7; i >= 0; --i) {
            buf_[pos_++] = static_cast<std::uint8_t>(v >> (i * 8));
        }
        return true;
    }

    [[nodiscard]] std::size_t size() const noexcept { return pos_; }
    [[nodiscard]] bool ok() const noexcept { return !overflow_; }

private:
    std::uint8_t* buf_;
    std::size_t   cap_;
    std::size_t   pos_ = 0;
    bool          overflow_ = false;
};

class Reader {
public:
    Reader(const std::uint8_t* buf, std::size_t len) : buf_(buf), len_(len) {}

    HFT_FORCE_INLINE bool u8(std::uint8_t& v) noexcept {
        if (HFT_UNLIKELY(pos_ + 1 > len_)) return false;
        v = buf_[pos_++];
        return true;
    }
    HFT_FORCE_INLINE bool u16(std::uint16_t& v) noexcept {
        std::uint8_t b0, b1;
        if (!u8(b0) || !u8(b1)) return false;
        v = static_cast<std::uint16_t>((b0 << 8) | b1);
        return true;
    }
    HFT_FORCE_INLINE bool u32(std::uint32_t& v) noexcept {
        std::uint8_t b[4];
        for (auto& x : b) { if (!u8(x)) return false; }
        v = (static_cast<std::uint32_t>(b[0]) << 24) |
            (static_cast<std::uint32_t>(b[1]) << 16) |
            (static_cast<std::uint32_t>(b[2]) << 8)  |
             static_cast<std::uint32_t>(b[3]);
        return true;
    }
    HFT_FORCE_INLINE bool u64(std::uint64_t& v) noexcept {
        std::uint64_t acc = 0;
        std::uint8_t x;
        for (int i = 0; i < 8; ++i) {
            if (!u8(x)) return false;
            acc = (acc << 8) | x;
        }
        v = acc;
        return true;
    }

    [[nodiscard]] std::size_t remaining() const noexcept { return len_ - pos_; }
    [[nodiscard]] std::size_t pos() const noexcept { return pos_; }

private:
    const std::uint8_t* buf_;
    std::size_t         len_;
    std::size_t         pos_ = 0;
};

// 编码一条完整消息（含帧头）。失败（缓冲不足）返回 0。
HFT_FORCE_INLINE std::size_t encode(std::uint8_t* buf, std::size_t cap,
                                    const Message& m) noexcept {
    const std::size_t total = message_wire_size(m.type);
    if (HFT_UNLIKELY(total > cap || total > 0xFFFF)) return 0;

    Writer w(buf, cap);
    if (!w.u16(static_cast<std::uint16_t>(total))) return 0;
    if (!w.u8(static_cast<std::uint8_t>(m.type))) return 0;
    if (!w.u64(m.hdr.seq)) return 0;
    if (!w.u64(m.hdr.ts)) return 0;

    bool ok = true;
    switch (m.type) {
        case MsgType::Heartbeat:
        case MsgType::SessionEnd:
            break;
        case MsgType::SessionStart:
            ok = w.u32(m.session_start.symbol);
            break;
        case MsgType::NewOrder:
            ok = w.u64(m.new_order.id)
              && w.u32(m.new_order.symbol)
              && w.u64(static_cast<std::uint64_t>(m.new_order.price))
              && w.u64(static_cast<std::uint64_t>(m.new_order.qty))
              && w.u8(static_cast<std::uint8_t>(m.new_order.side));
            break;
        case MsgType::CancelOrder:
            ok = w.u64(m.cancel_order.id);
            break;
        case MsgType::ReplaceOrder:
            ok = w.u64(m.replace_order.id)
              && w.u64(static_cast<std::uint64_t>(m.replace_order.new_price))
              && w.u64(static_cast<std::uint64_t>(m.replace_order.new_qty));
            break;
        case MsgType::Trade:
            ok = w.u64(m.trade.aggressor_id)
              && w.u64(m.trade.resting_id)
              && w.u64(static_cast<std::uint64_t>(m.trade.price))
              && w.u64(static_cast<std::uint64_t>(m.trade.qty));
            break;
        case MsgType::GapFill:
            ok = w.u64(m.gap_fill.from) && w.u64(m.gap_fill.to);
            break;
        default:
            return 0;
    }
    return ok ? w.size() : 0;
}

// 从报文游标处解码一条消息。成功返回消费字节数（>0）；
// 数据不足/畸形返回 0 —— 调用方应丢弃剩余报文（组播无重传语义内的自愈）。
HFT_FORCE_INLINE std::size_t decode(const std::uint8_t* buf, std::size_t len,
                                    Message& out) noexcept {
    if (HFT_UNLIKELY(len < kFrameHeaderSize)) return 0;

    Reader r(buf, len);
    std::uint16_t total;
    std::uint8_t  type_raw;
    if (!r.u16(total) || !r.u8(type_raw)) return 0;
    if (HFT_UNLIKELY(total < kFrameHeaderSize + kHeaderWireSize)) return 0;
    if (HFT_UNLIKELY(total > len)) return 0;

    const auto type = static_cast<MsgType>(type_raw);
    const std::size_t expect = message_wire_size(type);
    // 未知类型（expect 只含头）或长度与协议表不符 → 畸形。
    if (HFT_UNLIKELY(total != expect)) return 0;

    out.type = type;
    if (!r.u64(out.hdr.seq) || !r.u64(out.hdr.ts)) return 0;

    std::uint64_t v64; std::uint32_t v32; std::uint8_t v8;
    switch (type) {
        case MsgType::Heartbeat:
        case MsgType::SessionEnd:
            break;
        case MsgType::SessionStart:
            if (!r.u32(v32)) return 0;
            out.session_start.symbol = v32;
            break;
        case MsgType::NewOrder:
            if (!r.u64(out.new_order.id)) return 0;
            if (!r.u32(v32)) return 0;
            out.new_order.symbol = v32;
            if (!r.u64(v64)) return 0;
            out.new_order.price = static_cast<Price>(v64);
            if (!r.u64(v64)) return 0;
            out.new_order.qty = static_cast<Qty>(v64);
            if (!r.u8(v8)) return 0;
            if (HFT_UNLIKELY(v8 > 1)) return 0; // Side 越界 = 畸形
            out.new_order.side = static_cast<Side>(v8);
            break;
        case MsgType::CancelOrder:
            if (!r.u64(out.cancel_order.id)) return 0;
            break;
        case MsgType::ReplaceOrder:
            if (!r.u64(out.replace_order.id)) return 0;
            if (!r.u64(v64)) return 0;
            out.replace_order.new_price = static_cast<Price>(v64);
            if (!r.u64(v64)) return 0;
            out.replace_order.new_qty = static_cast<Qty>(v64);
            break;
        case MsgType::Trade:
            if (!r.u64(out.trade.aggressor_id)) return 0;
            if (!r.u64(out.trade.resting_id)) return 0;
            if (!r.u64(v64)) return 0;
            out.trade.price = static_cast<Price>(v64);
            if (!r.u64(v64)) return 0;
            out.trade.qty = static_cast<Qty>(v64);
            break;
        case MsgType::GapFill:
            if (!r.u64(out.gap_fill.from)) return 0;
            if (!r.u64(out.gap_fill.to)) return 0;
            break;
        default:
            // 未知类型字节：不接受、不解析，整条报文按畸形处理。
            return 0;
    }
    return total;
}

} // namespace hft::proto
