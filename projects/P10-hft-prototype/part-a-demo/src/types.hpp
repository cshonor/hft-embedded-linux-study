#pragma once

#include <cstdint>

// 价格用整数 tick，避免 double。demo 里 1 tick = 0.01 货币单位。
using Price = std::int64_t;
using Qty   = std::int64_t;

constexpr Price kInvalidPrice = -1;
constexpr Price kMaxPrice     = 1'000'000'000;

constexpr std::uint32_t kOwnerMarket = 0;
constexpr std::uint32_t kOwnerUs     = 1;

enum class Side : std::uint8_t { Buy, Sell };
enum class OrderType : std::uint8_t { Limit, Market, IOC, FOK };

enum class EventKind : std::uint8_t { Order, Cancel, Shutdown };

inline const char* side_str(Side s) {
    return s == Side::Buy ? "BUY" : "SELL";
}

struct Order {
    std::uint64_t id      = 0;
    std::uint32_t owner   = kOwnerMarket;
    Side          side    = Side::Buy;
    OrderType     type    = OrderType::Limit;
    Price         price   = 0;
    Qty           qty     = 0;
    std::uint64_t ts      = 0;
};

struct Trade {
    std::uint64_t taker_id    = 0;
    std::uint64_t maker_id    = 0;
    std::uint32_t taker_owner = 0;
    std::uint32_t maker_owner = 0;
    Side          taker_side  = Side::Buy;
    Price         price       = 0;
    Qty           qty         = 0;
};

struct Event {
    EventKind     kind        = EventKind::Order;
    Order         order{};
    std::uint64_t cancel_id   = 0;
    bool          run_strategy = false;
    std::uint64_t t0_ns       = 0;
};

