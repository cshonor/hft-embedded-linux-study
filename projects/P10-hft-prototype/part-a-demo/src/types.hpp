#pragma once

#include <cstdint>

/*
 * 新手先记住三件事：
 *
 * 1) 价格不用 double。0.1+0.2 在浮点里不等于 0.3，撮合会对不齐。
 *    这里 Price 是整数 tick：1 tick = 0.01，所以 100.00 元写成 10000。
 * 2) 买卖是两个方向：Buy 想买（出价），Sell 想卖（要价）。
 * 3) 市场上有两类人：owner=0 是「别人」（回放），owner=1 是「我们」（策略）。
 */

using Price = std::int64_t; // 价格，单位 tick
using Qty   = std::int64_t; // 数量，单位「张 / 股」，也用整数

constexpr Price kInvalidPrice = -1;          // 簿上没有买/卖时的哨兵
constexpr Price kMaxPrice     = 1'000'000'000; // 市价买单：当成「无限高」去吃卖盘

constexpr std::uint32_t kOwnerMarket = 0; // 回放里的其他人
constexpr std::uint32_t kOwnerUs     = 1; // 我们的做市策略

enum class Side : std::uint8_t { Buy, Sell };

/*
 * Limit  限价：能成交就成交，剩下来挂在簿上排队（提供流动性）
 * Market 市价：不指定价格，能吃多少吃多少，不挂单
 * IOC    Immediate-or-Cancel：限价立刻成交，剩余扔掉，不挂
 * FOK    Fill-or-Kill：要么一次全成，要么整单取消
 */
enum class OrderType : std::uint8_t { Limit, Market, IOC, FOK };

enum class EventKind : std::uint8_t { Order, Cancel, Shutdown };

inline const char* side_str(Side s) {
    return s == Side::Buy ? "BUY" : "SELL";
}

struct Order {
    std::uint64_t id      = 0;             // 每张单一个编号，撤单靠它
    std::uint32_t owner   = kOwnerMarket;  // 谁下的
    Side          side    = Side::Buy;
    OrderType     type    = OrderType::Limit;
    Price         price   = 0;
    Qty           qty     = 0;             // 剩余未成交量；撮合时会往下减
    std::uint64_t ts      = 0;
};

/*
 * 一笔成交。taker = 主动来吃的单（刚 submit 的）；
 * maker = 已经挂在簿上被吃掉的单。
 * 成交价用 maker 的挂单价（价格改善归 taker）。
 */
struct Trade {
    std::uint64_t taker_id    = 0;
    std::uint64_t maker_id    = 0;
    std::uint32_t taker_owner = 0;
    std::uint32_t maker_owner = 0;
    Side          taker_side  = Side::Buy;
    Price         price       = 0;
    Qty           qty         = 0;
};

/*
 * 回放线程 → 引擎线程 的消息。
 * run_strategy=true 表示「这一拍市场动作结束了，该让我们报价了」。
 * t0_ns 用来算事件在队列里等了多久。
 */
struct Event {
    EventKind     kind         = EventKind::Order;
    Order         order{};
    std::uint64_t cancel_id    = 0;
    bool          run_strategy = false;
    std::uint64_t t0_ns        = 0;
};
