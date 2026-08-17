#pragma once

#include "types.hpp"

#include <algorithm>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>

/*
 * 限价订单簿（LOB）= 买单簿 + 卖单簿。
 *
 *   买单 bids：价格从高到低（std::greater）——最优买 = begin()
 *   卖单 asks：价格从低到高（std::less）  ——最优卖 = begin()
 *
 * 同一价格上多张单用 list 排队：先到先成交（FIFO，笔记 Ch3.3）。
 *
 * 撮合三种结果（Ch3.2）：
 *   Best Price   买单先吃最便宜的卖
 *   Partial Fill 只吃到一部分，限价剩余再挂上
 *   No Match     价格碰不上，整单挂上
 *
 * 成交价 = 被吃那张挂单的价格（价格改善归主动方）。
 */
struct PriceLevel {
    std::list<Order> orders;
    Qty total = 0; // 这一档所有订单数量之和，避免每次遍历 list
};

// 撤单时：用订单 id 反查它在买还是卖、什么价格。

struct BookLoc {
    Side  side;
    Price price;
};

// 限价订单簿。demo 用 std::map，正确性优先；生产会换成数组网格做到 O(1) 摸 BBO。
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

        // 市价单不看价格：买单当成「无限高」，卖单当成 0，好走同一套 crosses()。
        if (order.type == OrderType::Market) {
            order.price = (order.side == Side::Buy) ? kMaxPrice : 0;
        }

        // FOK：先问「对手盘够不够」，不够整单作废，连一股都不成交。
        if (order.type == OrderType::FOK) {
            if (available_to_fill(order) < order.qty) {
                return {};
            }
            order.type = OrderType::IOC;
        }

        std::vector<Trade> trades;
        match(order, trades);

        // Limit 才挂剩余；Market / IOC 吃完就停，不排队。
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
        // 买单能吃到这档卖：买价 >= 卖价；卖单对称。
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
        auto it = opp.begin(); // 对手盘最优价
        while (incoming.qty > 0 && it != opp.end()) {
            const Price px = it->first;
            if (!crosses(incoming, px)) {
                break; // 价格碰不上，后面更差，不用看了
            }
            PriceLevel& lvl = it->second;
            auto oit = lvl.orders.begin(); // FIFO：队头先成交
            while (incoming.qty > 0 && oit != lvl.orders.end()) {
                if (incoming.owner != kOwnerMarket && oit->owner == incoming.owner) {
                    ++oit; // 自成交保护：跳过自己的挂单，继续找别人
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
        // 买单去吃卖盘；卖单去吃买盘。
        if (incoming.side == Side::Buy) {
            match_against(incoming, asks, trades);
        } else {
            match_against(incoming, bids, trades);
        }
    }

    void rest_order(const Order& o) {
        // push_back = 排到该价位队尾，保证时间优先。
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
