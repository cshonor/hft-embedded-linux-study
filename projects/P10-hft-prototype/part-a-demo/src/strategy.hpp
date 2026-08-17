#pragma once

#include "orderbook.hpp"
#include "types.hpp"

#include <cstdint>
#include <vector>

struct StrategyConfig {
    Qty   quote_size   = 10;
    Price half_spread  = 2;     // 基础半价差（tick）
    Price skew_per_pos = 1;     // 每 10 张库存偏斜 1 tick
    Qty   skew_unit    = 10;
    Price max_half     = 12;
};

// 双边做市：价差收入 − 库存偏斜 − 波动加宽。对应 Ch12.1。
class MarketMaker {
public:
    StrategyConfig cfg;
    Qty inventory = 0;
    std::int64_t cash_ticks = 0; // 以 tick·股 计的现金

    std::uint64_t live_bid_id = 0;
    std::uint64_t live_ask_id = 0;

    int fills = 0;
    Qty filled_qty = 0;
    Price last_mid = kInvalidPrice;
    double vol_ema = 0.0;

    void on_fill(const Trade& t) {
        const bool we_taker = t.taker_owner == kOwnerUs;
        const bool we_maker = t.maker_owner == kOwnerUs;
        if (!we_taker && !we_maker) {
            return;
        }

        Side our_side;
        if (we_taker) {
            our_side = t.taker_side;
        } else {
            our_side = (t.taker_side == Side::Buy) ? Side::Sell : Side::Buy;
        }

        if (our_side == Side::Buy) {
            inventory += t.qty;
            cash_ticks -= t.price * t.qty;
        } else {
            inventory -= t.qty;
            cash_ticks += t.price * t.qty;
        }
        ++fills;
        filled_qty += t.qty;
    }

    // 标记 PnL = 现金 + 库存 * mid
    std::int64_t mtm_pnl(Price mid) const {
        if (mid == kInvalidPrice) {
            return cash_ticks;
        }
        return cash_ticks + inventory * mid;
    }

    std::vector<Order> quote(const OrderBook& book, std::uint64_t& id_seq, std::uint64_t ts) {
        std::vector<Order> out;
        if (!book.has_bbo()) {
            return out;
        }

        const Price mid_now = book.mid();
        if (last_mid != kInvalidPrice) {
            const Price d = mid_now > last_mid ? (mid_now - last_mid) : (last_mid - mid_now);
            vol_ema = 0.9 * vol_ema + 0.1 * static_cast<double>(d);
        }
        last_mid = mid_now;

        Price half = cfg.half_spread + static_cast<Price>(vol_ema + 0.5);
        if (half < cfg.half_spread) {
            half = cfg.half_spread;
        }
        if (half > cfg.max_half) {
            half = cfg.max_half;
        }

        Price skew = 0;
        if (cfg.skew_unit > 0) {
            skew = (inventory / cfg.skew_unit) * cfg.skew_per_pos;
        }

        // 库存多 → 买价下移、卖价下移（更想卖）
        Price bid_px = mid_now - half - skew;
        Price ask_px = mid_now + half - skew;
        if (bid_px < 1) {
            bid_px = 1;
        }
        if (ask_px <= bid_px) {
            ask_px = bid_px + 1;
        }

        Order bid;
        bid.id    = ++id_seq;
        bid.owner = kOwnerUs;
        bid.side  = Side::Buy;
        bid.type  = OrderType::Limit;
        bid.price = bid_px;
        bid.qty   = cfg.quote_size;
        bid.ts    = ts;

        Order ask;
        ask.id    = ++id_seq;
        ask.owner = kOwnerUs;
        ask.side  = Side::Sell;
        ask.type  = OrderType::Limit;
        ask.price = ask_px;
        ask.qty   = cfg.quote_size;
        ask.ts    = ts;

        out.push_back(bid);
        out.push_back(ask);
        return out;
    }
};
