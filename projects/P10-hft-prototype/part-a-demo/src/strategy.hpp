#pragma once

#include "orderbook.hpp"
#include "types.hpp"

#include <cstdint>
#include <vector>

struct StrategyConfig {
    Qty   quote_size   = 10;
    Price half_spread  = 2;  // 中间价上下各挂 2 tick，这就是我们想赚的价差
    Price skew_per_pos = 1;  // 每囤 10 张，报价整体挪 1 tick
    Qty   skew_unit    = 10;
    Price max_half     = 12; // 波动再大也不把价差拉得无限宽
};

/*
 * 做市：同时挂买和卖，别人来成交我们就赚买卖价差。
 *
 *   PnL ≈ 成交量 × 价差  − 逆向选择  − 库存风险     （笔记 Ch12.1）
 *
 * 库存偏斜：手里货多了 → 买价/卖价一起下移，让市场更容易从我们这儿买走，
 * 从而把库存降下来。货空了则反过来。
 *
 * 注意：每 tick 撤旧挂新，在 FIFO 市场会丢掉队列位置。这是教学简化。
 */
class MarketMaker {
public:
    StrategyConfig cfg;
    Qty inventory = 0;           // >0 多头（买多了），<0 空头
    std::int64_t cash_ticks = 0; // 现金，单位 tick×数量

    std::uint64_t live_bid_id = 0; // 当前挂着的买/卖单号，下一拍要撤
    std::uint64_t live_ask_id = 0;

    int fills = 0;
    Qty filled_qty = 0;
    Price last_mid = kInvalidPrice;
    double vol_ema = 0.0; // 中间价变动的平滑值，用来把价差加宽

    void on_fill(const Trade& t) {
        const bool we_taker = t.taker_owner == kOwnerUs;
        const bool we_maker = t.maker_owner == kOwnerUs;
        if (!we_taker && !we_maker) {
            return; // 别人跟别人成交，与我们无关
        }

        // 我们是 taker：方向就是 taker_side。
        // 我们是 maker：对方买则我们在卖，对方卖则我们在买。
        Side our_side;
        if (we_taker) {
            our_side = t.taker_side;
        } else {
            our_side = (t.taker_side == Side::Buy) ? Side::Sell : Side::Buy;
        }

        if (our_side == Side::Buy) {
            inventory += t.qty;
            cash_ticks -= t.price * t.qty; // 花钱拿货
        } else {
            inventory -= t.qty;
            cash_ticks += t.price * t.qty; // 出货收钱
        }
        ++fills;
        filled_qty += t.qty;
    }

    // 把库存按现在的中间价标成现金：账面盈亏。
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

        // 库存多 → 两边报价下移（更想卖掉）；库存空 → 上移（更想买回）。
        Price bid_px = mid_now - half - skew;
        Price ask_px = mid_now + half - skew;
        if (bid_px < 1) {
            bid_px = 1;
        }
        if (ask_px <= bid_px) {
            ask_px = bid_px + 1; // 买价不能超过卖价，否则自己跟自己成交
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
