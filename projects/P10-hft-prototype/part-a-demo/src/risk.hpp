#pragma once

#include "orderbook.hpp"
#include "types.hpp"

enum class RiskReason : std::uint8_t {
    Ok = 0,
    KillSwitch,
    QtyCap,
    PriceBand,
    PositionCap,
    RateLimit,
};

inline const char* risk_str(RiskReason r) {
    switch (r) {
    case RiskReason::Ok:          return "ok";
    case RiskReason::KillSwitch:  return "kill";
    case RiskReason::QtyCap:      return "qty";
    case RiskReason::PriceBand:   return "band";
    case RiskReason::PositionCap: return "pos";
    case RiskReason::RateLimit:   return "rate";
    }
    return "?";
}

struct RiskConfig {
    Price band_ticks          = 80;   // 相对 mid 的最大偏离
    Qty   max_order_qty       = 50;
    Qty   max_position        = 150;
    int   max_orders_per_tick = 4;
};

// 热路径风控：几次比较，不分配、不调系统调用。对应 Ch10.1。
class RiskGate {
public:
    RiskConfig cfg;
    bool kill = false;

    int  orders_this_tick = 0;
    int  rejects          = 0;
    int  accept           = 0;

    void on_tick_start() { orders_this_tick = 0; }

    RiskReason check(const Order& o, const OrderBook& book, Qty inventory) {
        if (kill) {
            ++rejects;
            return RiskReason::KillSwitch;
        }
        if (o.qty <= 0 || o.qty > cfg.max_order_qty) {
            ++rejects;
            return RiskReason::QtyCap;
        }
        if (orders_this_tick >= cfg.max_orders_per_tick) {
            ++rejects;
            return RiskReason::RateLimit;
        }

        const Price mid = book.mid();
        if (o.type == OrderType::Limit && mid != kInvalidPrice) {
            const Price dist = o.price > mid ? (o.price - mid) : (mid - o.price);
            if (dist > cfg.band_ticks) {
                ++rejects;
                return RiskReason::PriceBand;
            }
        }

        Qty next = inventory;
        if (o.side == Side::Buy) {
            next += o.qty;
        } else {
            next -= o.qty;
        }
        const Qty abs_next = next >= 0 ? next : -next;
        const Qty abs_now  = inventory >= 0 ? inventory : -inventory;
        // 允许减仓；禁止把绝对仓位越撑越大
        if (abs_next > cfg.max_position && abs_next > abs_now) {
            ++rejects;
            return RiskReason::PositionCap;
        }

        ++orders_this_tick;
        ++accept;
        return RiskReason::Ok;
    }
};
