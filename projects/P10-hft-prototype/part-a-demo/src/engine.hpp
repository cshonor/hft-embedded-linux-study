#pragma once

#include "latency.hpp"
#include "orderbook.hpp"
#include "replay.hpp"
#include "risk.hpp"
#include "strategy.hpp"
#include "types.hpp"

#include <cstdio>
#include <thread>

/*
 * 引擎线程：从 SPSC 弹出事件，更新订单簿，必要时跑策略+风控+我们的报价。
 *
 * 一个 tick 的顺序（和实盘「先看行情再下单」一样）：
 *   别人的撤/挂/市价冲击  →  可能打到我们的旧单
 *   然后撤我们旧报价、按新 BBO 挂新买卖
 *   风控不过的单直接扔掉，不进撮合
 *
 * 两个延迟：
 *   queue_wait = 事件在环里排队（本机 WSL 往往是毫秒）
 *   latency    = 弹出之后纯计算（报价+风控+下单，通常几百纳秒）
 */
struct DemoStats {
    int ticks        = 0;
    int events       = 0;
    int our_fills    = 0;
    int risk_reject  = 0;
    int risk_accept  = 0;
    Qty inventory    = 0;
    std::int64_t pnl = 0;
    Price last_mid   = kInvalidPrice;
    Price last_fair  = 0;
};

class Engine {
public:
    OrderBook    book;
    MarketMaker  mm;
    RiskGate     risk;
    LatencyHist  latency;      // 策略+风控+下单 纯计算
    LatencyHist  queue_wait;   // 事件在 SPSC 里排队
    std::uint64_t next_id_ = 1'000'000'000ull; // 和回放的 id（从 1 起）错开，避免撞号
    int events = 0;
    int ticks  = 0;

    void apply_trades(const std::vector<Trade>& trades) {
        for (const auto& t : trades) {
            mm.on_fill(t);
        }
    }

    void handle(const Event& e) {
        ++events;
        if (e.kind == EventKind::Cancel) {
            book.cancel(e.cancel_id);
        } else if (e.kind == EventKind::Order) {
            apply_trades(book.submit(e.order));
        }

        if (!e.run_strategy) {
            return; // 这一笔只是市场更新，还没到我们决策的点
        }

        ++ticks;
        risk.on_tick_start();
        const std::uint64_t t_compute0 = ns_now();
        if (t_compute0 >= e.t0_ns) {
            queue_wait.add(t_compute0 - e.t0_ns);
        }

        // 每拍先撤旧报价再挂新的（教学用 cancel-replace）。
        if (mm.live_bid_id) {
            book.cancel(mm.live_bid_id);
            mm.live_bid_id = 0;
        }
        if (mm.live_ask_id) {
            book.cancel(mm.live_ask_id);
            mm.live_ask_id = 0;
        }

        const auto intents = mm.quote(book, next_id_, static_cast<std::uint64_t>(ticks));
        for (Order o : intents) {
            const RiskReason r = risk.check(o, book, mm.inventory);
            if (r != RiskReason::Ok) {
                continue;
            }
            apply_trades(book.submit(o));
            if (o.side == Side::Buy) {
                mm.live_bid_id = o.id;
            } else {
                mm.live_ask_id = o.id;
            }
        }

        const std::uint64_t t1 = ns_now();
        if (t1 >= t_compute0) {
            latency.add(t1 - t_compute0);
        }
    }

    void consume(EventRing& ring) {
        Event e;
        for (;;) {
            while (!ring.try_pop(e)) {
                // 空转等待。HFT 热路径宁愿空转，也不把核让给别人。
            }
            if (e.kind == EventKind::Shutdown) {
                break;
            }
            handle(e);
        }
    }

    DemoStats snapshot(Price fair) const {
        DemoStats s;
        s.ticks       = ticks;
        s.events      = events;
        s.our_fills   = mm.fills;
        s.risk_reject = risk.rejects;
        s.risk_accept = risk.accept;
        s.inventory   = mm.inventory;
        s.last_mid    = book.mid();
        s.pnl         = mm.mtm_pnl(s.last_mid);
        s.last_fair   = fair;
        return s;
    }

    void print_report(Price fair) const {
        const DemoStats s = snapshot(fair);
        const Price bb = book.best_bid();
        const Price ba = book.best_ask();

        std::printf("\n======== HFT demo report ========\n");
        std::printf("ticks=%d  events=%d  resting=%zu\n", s.ticks, s.events, book.resting());
        std::printf("BBO  bid=%lld.%02lld x %lld   ask=%lld.%02lld x %lld   mid=%lld.%02lld  spread=%lld tick\n",
                    whole(bb), frac(bb), static_cast<long long>(book.bid_qty()),
                    whole(ba), frac(ba), static_cast<long long>(book.ask_qty()),
                    whole(s.last_mid), frac(s.last_mid),
                    static_cast<long long>(book.spread()));
        std::printf("fair=%lld.%02lld  inventory=%lld  cash_ticks=%lld  mtm_PnL=%lld ticks (%.2f)\n",
                    whole(s.last_fair), frac(s.last_fair),
                    static_cast<long long>(s.inventory),
                    static_cast<long long>(mm.cash_ticks),
                    static_cast<long long>(s.pnl),
                    static_cast<double>(s.pnl) / 100.0);
        std::printf("our fills=%d  filled_qty=%lld  risk accept=%d  reject=%d\n",
                    s.our_fills,
                    static_cast<long long>(mm.filled_qty),
                    s.risk_accept,
                    s.risk_reject);
        latency.print("compute  (quote+risk+submit)");
        queue_wait.print("queue    (SPSC wait)");
        std::printf("=================================\n");
        std::printf("PnL 单位：1.00 = 100 tick。正数 ≈ 赚到价差；跳价密集时库存会被打偏。\n");
    }

private:
    static long long whole(Price px) {
        if (px == kInvalidPrice) {
            return 0;
        }
        return static_cast<long long>(px / 100);
    }
    static long long frac(Price px) {
        if (px == kInvalidPrice) {
            return 0;
        }
        Price r = px % 100;
        if (r < 0) {
            r = -r;
        }
        return static_cast<long long>(r);
    }
};

inline DemoStats run_demo(const ReplayConfig& cfg) {
    EventRing ring;
    Engine engine;
    Replay replay(cfg);

    // 生产者：回放线程往环里塞事件。消费者：本线程弹出并处理。
    std::thread producer([&]() { replay.run(ring); });
    engine.consume(ring);
    producer.join();

    engine.print_report(replay.fair());
    return engine.snapshot(replay.fair());
}
