#pragma once

/*
 * types.hpp — 全系统统一的基础类型。
 *
 * 生产惯例：
 *   - 价格一律定点整数（tick 计数），浮点只出现在报表，永不进热路径。
 *     double 的精度陷阱（0.1 不可表示）在撮合/风控比较里是会吃钱的 bug。
 *   - 数量有符号：做空/净库存用负数表示比 "side + unsigned" 少一次分支。
 *   - OrderId / SeqNum 用 64 位：交易所级别消息量下 32 位会回绕。
 */

#include <cstdint>

namespace hft {

using Price   = std::int64_t;   // 单位 tick（1 tick = 0.01 元/美元，视品种配置）
using Qty     = std::int64_t;   // 有符号：正=多头/买量，负=空头/卖量
using OrderId = std::uint64_t;  // 全局唯一订单号
using SeqNum  = std::uint64_t;  // 行情/回报序列号
using Symbol  = std::uint32_t;  // 合约内部编码（启动时从配置映射，热路径不碰字符串）
using TsNs    = std::uint64_t;  // 纳秒时间戳（CLOCK_MONOTONIC 或硬件时间源）

inline constexpr Price   kInvalidPrice   = 0;    // 合法价格恒 > 0
inline constexpr OrderId kInvalidOrderId = 0;
inline constexpr SeqNum  kInvalidSeq     = 0;    // 序列号从 1 开始（ITCH 惯例）

enum class Side : std::uint8_t {
    Buy  = 0,
    Sell = 1,
};

// 订单簿深度上限等编译期容量集中在这里，防止魔数散落各处。
struct Limits {
    static constexpr std::size_t kMaxOrders      = 1u << 20; // 单合约驻留订单容量
    static constexpr std::size_t kMaxPriceLevels = 1u << 16; // 价位数上限（tick 域窗口）
    static constexpr std::size_t kUdpMaxPayload  = 1472;     // 不分片的 UDP 安全负载
};

} // namespace hft
