#pragma once

#include "orderbook.hpp"
#include "risk.hpp"
#include "types.hpp"

#include <cstdio>

/*
 * 不跑回放，只拿固定订单验证撮合/风控有没有写错。
 * 价格写成 10300 表示 103.00 元（×100 tick）。
 * 对照笔记 Ch3.2 三种场景 + Ch3.3 FIFO + Ch10.1 风控。
 */

inline int fail(const char* name, const char* why) {
    std::printf("FAIL  %s: %s\n", name, why);
    return 1;
}

inline int pass(const char* name) {
    std::printf("PASS  %s\n", name);
    return 0;
}

inline int self_test() {
    int fails = 0;
    std::uint64_t id = 1;

    auto mk = [&](Side side, Price px, Qty qty, OrderType ty = OrderType::Limit,
                  std::uint32_t owner = kOwnerMarket) {
        Order o;
        o.id    = ++id;
        o.owner = owner;
        o.side  = side;
        o.type  = ty;
        o.price = px;
        o.qty   = qty;
        return o;
    };

    // 1. Best Price：买 105，簿上卖 103 与 104 → 先 103
    {
        OrderBook b;
        b.submit(mk(Side::Sell, 10400, 50));
        b.submit(mk(Side::Sell, 10300, 200));
        auto t = b.submit(mk(Side::Buy, 10500, 100));
        if (t.size() != 1 || t[0].price != 10300 || t[0].qty != 100) {
            fails += fail("best-price", "should fill 100 @ 103.00");
        } else {
            pass("best-price");
        }
    }

    // 2. Partial Fill：买 4，最优卖 1 → 成交 1，剩余挂买
    {
        OrderBook b;
        b.submit(mk(Side::Sell, 10000, 1));
        auto t = b.submit(mk(Side::Buy, 10000, 4));
        if (t.size() != 1 || t[0].qty != 1 || b.best_bid() != 10000 || b.bid_qty() != 3) {
            fails += fail("partial-fill", "expect fill 1, rest bid qty 3");
        } else {
            pass("partial-fill");
        }
    }

    // 3. No Match：买 98 vs 卖 99
    {
        OrderBook b;
        b.submit(mk(Side::Sell, 9900, 10));
        auto t = b.submit(mk(Side::Buy, 9800, 10));
        if (!t.empty() || b.best_bid() != 9800 || b.best_ask() != 9900) {
            fails += fail("no-match", "should rest both sides");
        } else {
            pass("no-match");
        }
    }

    // 4. FIFO：同价两笔卖，先到先成交
    {
        OrderBook b;
        auto a = mk(Side::Sell, 10000, 10);
        auto c = mk(Side::Sell, 10000, 10);
        b.submit(a);
        b.submit(c);
        auto t = b.submit(mk(Side::Buy, 10000, 10));
        if (t.size() != 1 || t[0].maker_id != a.id) {
            fails += fail("fifo", "first resting sell should fill first");
        } else {
            pass("fifo");
        }
    }

    // 5. Cancel
    {
        OrderBook b;
        auto a = mk(Side::Sell, 10000, 10);
        b.submit(a);
        if (!b.cancel(a.id) || b.best_ask() != kInvalidPrice) {
            fails += fail("cancel", "cancel should remove the only ask");
        } else {
            pass("cancel");
        }
    }

    // 6. Market order 吃穿两档
    {
        OrderBook b;
        b.submit(mk(Side::Sell, 10000, 5));
        b.submit(mk(Side::Sell, 10100, 5));
        auto t = b.submit(mk(Side::Buy, 0, 8, OrderType::Market));
        Qty q = 0;
        for (auto& x : t) {
            q += x.qty;
        }
        if (q != 8 || t[0].price != 10000 || t[1].price != 10100) {
            fails += fail("market", "should walk 5@100 then 3@101");
        } else {
            pass("market");
        }
    }

    // 7. IOC 剩余不挂
    {
        OrderBook b;
        b.submit(mk(Side::Sell, 10000, 3));
        auto t = b.submit(mk(Side::Buy, 10000, 10, OrderType::IOC));
        if (t.size() != 1 || t[0].qty != 3 || b.has_bbo()) {
            fails += fail("ioc", "remainder must not rest");
        } else {
            pass("ioc");
        }
    }

    // 8. FOK 量不够则全部撤销
    {
        OrderBook b;
        b.submit(mk(Side::Sell, 10000, 3));
        auto t = b.submit(mk(Side::Buy, 10000, 10, OrderType::FOK));
        if (!t.empty() || b.best_ask() != 10000) {
            fails += fail("fok", "must reject entirely when size insufficient");
        } else {
            pass("fok");
        }
    }

    // 9. STP 自成交
    {
        OrderBook b;
        b.submit(mk(Side::Sell, 10000, 10, OrderType::Limit, kOwnerUs));
        auto t = b.submit(mk(Side::Buy, 10000, 10, OrderType::Limit, kOwnerUs));
        if (!t.empty() || b.best_bid() != 10000 || b.best_ask() != 10000) {
            fails += fail("stp", "same owner must not match");
        } else {
            pass("stp");
        }
    }

    // 10. 风控价格带
    {
        OrderBook b;
        b.submit(mk(Side::Buy, 10000, 10));
        b.submit(mk(Side::Sell, 10020, 10));
        RiskGate g;
        g.cfg.band_ticks = 5;
        Order far = mk(Side::Buy, 10100, 1);
        far.owner = kOwnerUs;
        if (g.check(far, b, 0) != RiskReason::PriceBand) {
            fails += fail("risk-band", "order 80 ticks from mid should reject");
        } else {
            pass("risk-band");
        }
    }

    // 11. 风控持仓：加仓拒绝、减仓放行
    {
        OrderBook b;
        b.submit(mk(Side::Buy, 10000, 10));
        b.submit(mk(Side::Sell, 10020, 10));
        RiskGate g;
        g.cfg.max_position = 10;
        Order buy = mk(Side::Buy, 10000, 10);
        buy.owner = kOwnerUs;
        if (g.check(buy, b, 10) != RiskReason::PositionCap) {
            fails += fail("risk-pos-add", "increasing |pos| beyond cap");
        } else {
            pass("risk-pos-add");
        }
        g.on_tick_start();
        Order sell = mk(Side::Sell, 10020, 10);
        sell.owner = kOwnerUs;
        if (g.check(sell, b, 10) != RiskReason::Ok) {
            fails += fail("risk-pos-reduce", "reducing inventory must pass");
        } else {
            pass("risk-pos-reduce");
        }
    }

    std::printf("self-test %s  (%d failed)\n", fails ? "FAILED" : "OK", fails);
    return fails;
}
