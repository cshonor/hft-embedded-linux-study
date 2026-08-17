#pragma once

#include "types.hpp"

#include <algorithm>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>

struct PriceLevel {
    std::list<Order> orders;
    Qty total = 0;
};

struct BookLoc {
    Side  side;
    Price price;
};

// 限价订单簿：买单降序、卖单升序；同价 FIFO。
// 语义对齐 14-hft-engineering Ch3.2 / Ch3.3 与 P8 Phase 1。
class OrderBook {
public:
    using BidMap = std::map<Price, PriceLevel, std::greater<Price>>;
    using AskMap = std::map<Price, PriceLevel, std::less<Price>>;

    BidMap bids;
    AskMap asks;

    Price best_bid() const { return bids.empty() ? kInvalidPrice : bids.begin()->first; }
    Price best_ask() const { return asks.empty() ? kInvalidPrice : asks.begin()->first; }

    bool has_bbo() const { return !bids.empty() && !asks.empty(); }

    Price mid() const {
        if (!has_bbo()) {
            return kInvalidPrice;
        }
        return (best_bid() + best_ask()) / 2;
    }

    Price spread() const {
        if (!has_bbo()) {
            return kInvalidPrice;
        }
        return best_ask() - best_bid();
    }

    Qty bid_qty() const { return bids.empty() ? 0 : bids.begin()->second.total; }
    Qty ask_qty() const { return asks.empty() ? 0 : asks.begin()->second.total; }

    std::size_t resting() const { return index_.size(); }

    std::vector<Trade> submit(Order order) {
        if (order.qty <= 0) {
            return {};
        }

        if (order.type == OrderType::Market) {
            order.price = (order.side == Side::Buy) ? kMaxPrice : 0;
        }

        if (order.type == OrderType::FOK) {
            if (available_to_fill(order) < order.qty) {
                return {};
            }
            order.type = OrderType::IOC;
        }

        std::vector<Trade> trades;
        match(order, trades);

        const bool rest = (order.type == OrderType::Limit) && order.qty > 0;
        if (rest) {
            rest_order(order);
        }
        return trades;
    }

    bool cancel(std::uint64_t id) {
        auto iit = index_.find(id);
        if (iit == index_.end()) {
            return false;
        }
        const BookLoc loc = iit->second;
        if (loc.side == Side::Buy) {
            return erase_from(bids, loc.price, id);
        }
        return erase_from(asks, loc.price, id);
    }

    Qty available_to_fill(const Order& o) const {
        Qty avail = 0;
        if (o.side == Side::Buy) {
            for (const auto& kv : asks) {
                if (o.type != OrderType::Market && o.price < kv.first) {
                    break;
                }
                avail += qty_from_others(kv.second, o.owner);
                if (avail >= o.qty) {
                    return o.qty;
                }
            }
        } else {
            for (const auto& kv : bids) {
                if (o.type != OrderType::Market && o.price > kv.first) {
                    break;
                }
                avail += qty_from_others(kv.second, o.owner);
                if (avail >= o.qty) {
                    return o.qty;
                }
            }
        }
        return avail;
    }

private:
    std::unordered_map<std::uint64_t, BookLoc> index_;

    static Qty qty_from_others(const PriceLevel& lvl, std::uint32_t owner) {
        Qty q = 0;
        for (const auto& o : lvl.orders) {
            // 市场侧 owner=0 表示「许多人」，彼此可以成交；只对自己的策略单做 STP。
            if (owner != kOwnerMarket && o.owner == owner) {
                continue;
            }
            q += o.qty;
        }
        return q;
    }

    static bool crosses(const Order& o, Price opp) {
        if (o.side == Side::Buy) {
            return o.price >= opp;
        }
        return o.price <= opp;
    }

    static Trade make_trade(const Order& taker, const Order& maker, Price px, Qty q) {
        Trade t;
        t.taker_id    = taker.id;
        t.maker_id    = maker.id;
        t.taker_owner = taker.owner;
        t.maker_owner = maker.owner;
        t.taker_side  = taker.side;
        t.price       = px;
        t.qty         = q;
        return t;
    }

    template <typename Map>
    void match_against(Order& incoming, Map& opp, std::vector<Trade>& trades) {
        auto it = opp.begin();
        while (incoming.qty > 0 && it != opp.end()) {
            const Price px = it->first;
            if (!crosses(incoming, px)) {
                break;
            }
            PriceLevel& lvl = it->second;
            auto oit = lvl.orders.begin();
            while (incoming.qty > 0 && oit != lvl.orders.end()) {
                if (incoming.owner != kOwnerMarket && oit->owner == incoming.owner) {
                    ++oit; // STP：只跳过我方自成交
                    continue;
                }
                const Qty q = std::min(incoming.qty, oit->qty);
                trades.push_back(make_trade(incoming, *oit, px, q));
                incoming.qty -= q;
                oit->qty -= q;
                lvl.total -= q;
                if (oit->qty == 0) {
                    index_.erase(oit->id);
                    oit = lvl.orders.erase(oit);
                } else {
                    ++oit;
                }
            }
            if (lvl.orders.empty()) {
                it = opp.erase(it);
            } else {
                ++it; // 本档只剩自己的单，看下一档
            }
        }
    }

    void match(Order& incoming, std::vector<Trade>& trades) {
        if (incoming.side == Side::Buy) {
            match_against(incoming, asks, trades);
        } else {
            match_against(incoming, bids, trades);
        }
    }

    void rest_order(const Order& o) {
        if (o.side == Side::Buy) {
            PriceLevel& lvl = bids[o.price];
            lvl.orders.push_back(o);
            lvl.total += o.qty;
        } else {
            PriceLevel& lvl = asks[o.price];
            lvl.orders.push_back(o);
            lvl.total += o.qty;
        }
        index_[o.id] = BookLoc{o.side, o.price};
    }

    template <typename Map>
    bool erase_from(Map& m, Price px, std::uint64_t id) {
        auto mit = m.find(px);
        if (mit == m.end()) {
            return false;
        }
        PriceLevel& lvl = mit->second;
        for (auto oit = lvl.orders.begin(); oit != lvl.orders.end(); ++oit) {
            if (oit->id == id) {
                lvl.total -= oit->qty;
                lvl.orders.erase(oit);
                index_.erase(id);
                if (lvl.orders.empty()) {
                    m.erase(mit);
                }
                return true;
            }
        }
        return false;
    }
};
