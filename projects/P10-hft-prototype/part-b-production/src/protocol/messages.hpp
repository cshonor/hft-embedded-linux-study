#pragma once

/*
 * messages.hpp — 线协议消息定义（ITCH 风格）。
 *
 * 设计决定（与 part-a demo 的本质区别）：
 *   - 线上字节序 = 大端（ITCH/OUCH/SBE 惯例），解码后引擎内一律宿主机序；
 *   - 定长消息、字段逐个序列化，**不**用 packed struct 直接强转——
 *     packed cast 在未对齐地址上是 UB，且把线格式和内存布局耦合死了；
 *   - 每个 payload 是平凡可复制的原生结构体，热路径 memcpy 安全；
 *   - 线尺寸用 constexpr 常量锁定，测试里 static_assert 级别校验，
 *     协议改动必须过测试，杜绝"加了字段忘改重放端"。
 *
 * 帧格式：
 *   +--------+------------------+-----------------+
 *   | u16 总长 | u8 MsgType      | payload (变长)   |
 *   +--------+------------------+-----------------+
 *   总长含头自身。UDP 一个数据报可装多条消息（交易所组播惯例）。
 */

#include <cstdint>

#include "common/types.hpp"

namespace hft::proto {

inline constexpr std::size_t kFrameHeaderSize = 3; // u16 len + u8 type

enum class MsgType : std::uint8_t {
    Heartbeat    = 'H', // 保活：无行情时段确认流活着
    SessionStart = 'S', // 交易日开始：重置序列号状态
    SessionEnd   = 'E',
    NewOrder     = 'A', // 挂单（book 增量）
    CancelOrder  = 'X', // 撤单
    ReplaceOrder = 'R', // 改单（价/量）
    Trade        = 'T', // 成交（吃单扫量，book 需减量）
    GapFill      = 'G', // 重传服务器告知"该区间无需恢复"（心跳类跳号）
};

// ---------- 消息头（解码后通用字段） ----------
struct Header {
    SeqNum seq;  // 流内单调递增，gap 检测的依据
    TsNs   ts;   // 发送方时间戳（ns）
};
inline constexpr std::size_t kHeaderWireSize = 8 + 8;

// ---------- 各 payload（宿主机序，平凡类型） ----------
struct SessionStart { Symbol symbol; };
inline constexpr std::size_t kSessionStartWireSize = 4;

struct NewOrder {
    OrderId id;
    Symbol  symbol;
    Price   price;
    Qty     qty;    // 恒正
    Side    side;
};
// 8 + 4 + 8 + 8 + 1
inline constexpr std::size_t kNewOrderWireSize = 29;

struct CancelOrder {
    OrderId id;
};
inline constexpr std::size_t kCancelOrderWireSize = 8;

struct ReplaceOrder {
    OrderId id;
    Price   new_price;
    Qty     new_qty; // 恒正
};
inline constexpr std::size_t kReplaceOrderWireSize = 24;

struct Trade {
    OrderId aggressor_id; // 吃单方订单（方向 = 成交方向）
    OrderId resting_id;   // 挂单方订单（book 减量目标）
    Price   price;
    Qty     qty;          // 恒正
};
inline constexpr std::size_t kTradeWireSize = 32;

struct GapFill {
    SeqNum from; // 被跳过的区间 [from, to]，接收端直接推进期望值
    SeqNum to;
};
inline constexpr std::size_t kGapFillWireSize = 16;

// ---------- 统一消息视图（tagged union，零堆分配） ----------
struct Message {
    MsgType type;
    Header  hdr;
    union {
        SessionStart session_start;
        NewOrder     new_order;
        CancelOrder  cancel_order;
        ReplaceOrder replace_order;
        Trade        trade;
        GapFill      gap_fill;
    };
    // Heartbeat / SessionEnd 无 payload。
};

// 各类型线尺寸（payload 部分，不含帧头/公共头），codec 与测试共用。
constexpr std::size_t payload_wire_size(MsgType t) noexcept {
    switch (t) {
        case MsgType::Heartbeat:    return 0;
        case MsgType::SessionStart: return kSessionStartWireSize;
        case MsgType::SessionEnd:   return 0;
        case MsgType::NewOrder:     return kNewOrderWireSize;
        case MsgType::CancelOrder:  return kCancelOrderWireSize;
        case MsgType::ReplaceOrder: return kReplaceOrderWireSize;
        case MsgType::Trade:        return kTradeWireSize;
        case MsgType::GapFill:      return kGapFillWireSize;
    }
    return 0;
}

// 一条消息的完整线长 = 帧头 + 公共头 + payload。
constexpr std::size_t message_wire_size(MsgType t) noexcept {
    return kFrameHeaderSize + kHeaderWireSize + payload_wire_size(t);
}

} // namespace hft::proto
