#pragma once

#include "spsc_ring.hpp"
#include "types.hpp"

#include <chrono>
#include <cstdint>
#include <random>

struct ReplayConfig {
    int   ticks       = 20000;
    Price start_fair  = 10000; // 100.00
    Price other_half  = 4;     // 「别人」做市更宽
    Qty   other_size  = 40;
    int   jump_every  = 80;    // 周期性跳价，制造逆向选择
    Price jump_ticks  = 12;
    double hit_prob   = 0.35;
    Qty   hit_max     = 12;
    std::uint32_t seed = 1;
};

using EventRing = SpscRing<Event, 65536>;

inline std::uint64_t ns_now() {
    using clock = std::chrono::steady_clock;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now().time_since_epoch()).count());
}

class Replay {
public:
    ReplayConfig cfg;

    explicit Replay(ReplayConfig c) : cfg(c), rng_(c.seed), fair_(c.start_fair) {}

    void run(EventRing& ring) {
        std::uniform_int_distribution<int> walk(-1, 1);
        std::uniform_real_distribution<double> uni(0.0, 1.0);
        std::uniform_int_distribution<int> hit_qty(1, static_cast<int>(cfg.hit_max));
        std::bernoulli_distribution buy_side(0.5);

        std::uint64_t other_bid = 0;
        std::uint64_t other_ask = 0;

        for (int i = 0; i < cfg.ticks; ++i) {
            fair_ += walk(rng_);
            if (fair_ < 1000) {
                fair_ = 1000;
            }
            if (cfg.jump_every > 0 && (i + 1) % cfg.jump_every == 0) {
                const int dir = buy_side(rng_) ? 1 : -1;
                fair_ += dir * cfg.jump_ticks;
            }

            if (other_bid) {
                push_cancel(ring, other_bid);
            }
            if (other_ask) {
                push_cancel(ring, other_ask);
            }

            other_bid = ++id_seq_;
            other_ask = ++id_seq_;

            Order bid;
            bid.id    = other_bid;
            bid.owner = kOwnerMarket;
            bid.side  = Side::Buy;
            bid.type  = OrderType::Limit;
            bid.price = fair_ - cfg.other_half;
            bid.qty   = cfg.other_size;
            bid.ts    = static_cast<std::uint64_t>(i);
            if (bid.price < 1) {
                bid.price = 1;
            }

            Order ask;
            ask.id    = other_ask;
            ask.owner = kOwnerMarket;
            ask.side  = Side::Sell;
            ask.type  = OrderType::Limit;
            ask.price = fair_ + cfg.other_half;
            ask.qty   = cfg.other_size;
            ask.ts    = static_cast<std::uint64_t>(i);

            const bool will_hit = uni(rng_) < cfg.hit_prob;
            push_order(ring, bid, false);
            push_order(ring, ask, !will_hit);

            if (will_hit) {
                Order hit;
                hit.id    = ++id_seq_;
                hit.owner = kOwnerMarket;
                hit.side  = buy_side(rng_) ? Side::Buy : Side::Sell;
                hit.type  = OrderType::Market;
                hit.price = 0;
                hit.qty   = hit_qty(rng_);
                hit.ts    = static_cast<std::uint64_t>(i);
                push_order(ring, hit, true);
            }
        }

        Event end;
        end.kind  = EventKind::Shutdown;
        end.t0_ns = ns_now();
        spin_push(ring, end);
    }

    Price fair() const { return fair_; }

private:
    std::mt19937 rng_;
    Price fair_;
    std::uint64_t id_seq_ = 1;

    static void spin_push(EventRing& ring, const Event& e) {
        while (!ring.try_push(e)) {
        }
    }

    static void push_order(EventRing& ring, const Order& o, bool run_strategy) {
        Event e;
        e.kind         = EventKind::Order;
        e.order        = o;
        e.run_strategy = run_strategy;
        e.t0_ns        = ns_now();
        spin_push(ring, e);
    }

    static void push_cancel(EventRing& ring, std::uint64_t id) {
        Event e;
        e.kind         = EventKind::Cancel;
        e.cancel_id    = id;
        e.run_strategy = false;
        e.t0_ns        = ns_now();
        spin_push(ring, e);
    }
};
