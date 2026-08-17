#pragma once

#include "orderbook.hpp"
#include "types.hpp"

enum class RiskReason : std::uint8_t {
    Ok = 0,
    KillSwitch,  // 人工/异常总开关，全部拒
    QtyCap,      // 单笔太大
    PriceBand,   // 离中间价太远（防乌龙指）
    PositionCap, // 仓位已经顶满，还想往同方向加
    RateLimit,   // 这一拍下单太勤
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
    Price band_ticks          = 80;  // 报价不能偏离 mid 超过这么多 tick
    Qty   max_order_qty       = 50;
    Qty   max_position        = 150;
    int   max_orders_per_tick = 4;
};

/*
 * 本地风控：策略再怎么 bug，单子也先过这一关才进撮合。
 * 对应笔记 Ch10.1——「违规单不出门」，不等交易所拒绝。
 * 热路径只做几次 if，不 new、不加锁、不调系统调用。
 */
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
        // 已经超限时，只许减仓（绝对仓位变小），不许再加。
        if (abs_next > cfg.max_position && abs_next > abs_now) {
            ++rejects;
            return RiskReason::PositionCap;
        }

        ++orders_this_tick;
        ++accept;
        return RiskReason::Ok;
    }
};
