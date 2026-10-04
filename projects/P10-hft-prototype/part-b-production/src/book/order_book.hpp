#pragma once

/*
 * order_book.hpp — 生产形态限价订单簿（单合约）。
 *
 * 结构选型（对比 part-a demo 的红黑树思路）：
 *   - 价位：数组 + 两级位图。tick 域有界（价格带），O(1) 定位，
 *     无指针追逐；最优价缓存 + 价位打空时位图重扫。
 *   - 价位内队列：侵入式双向链表，FIFO = price-time priority。
 *   - 订单定位：id → Order* 开放桶哈希（链地址），O(1) 撤单。
 *   - 内存：Pool 预分配，热路径零堆分配；耗尽返回 PoolExhausted。
 *
 * 撮合语义：
 *   - 限价单：价格优于等于对手方最优价即成交，成交价 = 挂单方价格
 *     （价格优先，挂单方享受价格改善）；
 *   - 市价单：不设价格保护，扫到哪儿算哪儿，剩余量**不驻留**（IOC 语义）；
 *   - 改单：降价量（同价减量）保留排队位置；改价或加量 = 撤旧挂新，
 *     丢失排队位置（交易所通行规则）；
 *   - 自成交防护（STP）不在簿内做——那是网关/风控层的职责，簿保持纯粹。
 *
 * 所有公共方法 noexcept，错误走 BookError 返回值。
 */

#include <cstddef>
#include <cstdint>
#include <memory>

#include "common/compiler.hpp"
#include "common/types.hpp"
#include "book/level_bitmap.hpp"
#include "book/memory_pool.hpp"
#include "protocol/messages.hpp"

namespace hft {

enum class BookError : std::uint8_t {
    Ok = 0,
    BadPrice,      // 价越出窗口 / 非法价
    BadQty,        // 量 <= 0
    DuplicateId,   // id 已在簿中
    UnknownId,     // 撤/改不存在的单
    PoolExhausted, // 订单容量耗尽（生产：拒单 + 告警）
};

enum class OrderType : std::uint8_t {
    Limit,
    Market,        // 剩余量 IOC，不驻留
};

struct Order {
    OrderId id;
    Price   price;
    Qty     qty;   // 剩余量，驻留期间恒 > 0
    Side    side;
    Order*  prev;  // 价位内 FIFO 链
    Order*  next;
    Order*  hnext; // id 索引冲突链
};

template <std::size_t MaxOrders, std::size_t NumLevels>
class OrderBook {
    static_assert((MaxOrders & (MaxOrders - 1)) == 0,
                  "MaxOrders 必须是 2 的幂（id 索引用掩码取桶）");

    struct Level {
        Order* head  = nullptr;
        Order* tail  = nullptr;
        Qty    total = 0; // 该价位挂单总量（深度行情要用）
    };

public:
    explicit OrderBook(Price base_price) noexcept
        : base_(base_price),
          levels_(new Level[NumLevels]),
          buckets_(new Order*[kIndexCap]()) {}

    // ---- 行情侧操作（book 增量） ----

    // 挂限价/市价单。成交通过 on_trade(const proto::Trade&) 逐笔回调。
    template <typename Sink>
    BookError submit(OrderId id, Side side, Price price, Qty qty,
                     OrderType type, Sink&& on_trade) noexcept {
        if (HFT_UNLIKELY(qty <= 0)) return BookError::BadQty;
        if (HFT_UNLIKELY(id == kInvalidOrderId || index_find(id) != nullptr)) {
            return BookError::DuplicateId;
        }

        if (type == OrderType::Market) {
            // 市价：价格保护拉到窗口尽头。
            price = (side == Side::Buy) ? max_price() : min_price();
        } else if (HFT_UNLIKELY(!price_in_window(price))) {
            return BookError::BadPrice;
        }

        // 撮合循环：扫对手方价位，FIFO 成交。
        if (side == Side::Buy) {
            match<Side::Sell>(id, price, qty, on_trade);
        } else {
            match<Side::Buy>(id, price, qty, on_trade);
        }

        // 剩余量驻留（市价单不驻留）。
        if (qty > 0 && type == OrderType::Limit) {
            Order* o = pool_.alloc();
            if (HFT_UNLIKELY(o == nullptr)) return BookError::PoolExhausted;
            o->id = id; o->price = price; o->qty = qty; o->side = side;
            rest(o);
        }
        return BookError::Ok;
    }

    BookError cancel(OrderId id) noexcept {
        Order* o = index_find(id);
        if (HFT_UNLIKELY(o == nullptr)) return BookError::UnknownId;
        unrest(o);
        return BookError::Ok;
    }

    // 改单。同价减量保留位置；其余 = 撤旧挂新（成交照样回调）。
    template <typename Sink>
    BookError replace(OrderId id, Price new_price, Qty new_qty,
                      Sink&& on_trade) noexcept {
        if (HFT_UNLIKELY(new_qty <= 0)) return BookError::BadQty;
        if (HFT_UNLIKELY(!price_in_window(new_price))) return BookError::BadPrice;
        Order* o = index_find(id);
        if (HFT_UNLIKELY(o == nullptr)) return BookError::UnknownId;

        if (new_price == o->price && new_qty <= o->qty) {
            level_at(o->price).total -= (o->qty - new_qty);
            o->qty = new_qty;
            return BookError::Ok;
        }
        const Side side = o->side;
        unrest(o);
        return submit(id, side, new_price, new_qty, OrderType::Limit,
                      static_cast<Sink&&>(on_trade));
    }

    // ---- BBO / 深度访问 ----

    [[nodiscard]] bool  has_bid() const noexcept { return has_bid_; }
    [[nodiscard]] bool  has_ask() const noexcept { return has_ask_; }
    [[nodiscard]] Price best_bid() const noexcept { return best_bid_; } // has_bid() 时才有效
    [[nodiscard]] Price best_ask() const noexcept { return best_ask_; }
    [[nodiscard]] Qty   level_qty(Price p) const noexcept {
        return price_in_window(p) ? levels_[static_cast<std::size_t>(p - base_)].total : 0;
    }
    [[nodiscard]] Qty best_bid_qty() const noexcept {
        return has_bid_ ? level_at(best_bid_).total : 0;
    }
    [[nodiscard]] Qty best_ask_qty() const noexcept {
        return has_ask_ ? level_at(best_ask_).total : 0;
    }

    [[nodiscard]] std::size_t resting() const noexcept { return resting_; }
    [[nodiscard]] std::size_t pool_available() const noexcept { return pool_.available(); }
    [[nodiscard]] Price base_price() const noexcept { return base_; }
    [[nodiscard]] Price min_price() const noexcept { return base_; }
    [[nodiscard]] Price max_price() const noexcept { return base_ + static_cast<Price>(NumLevels) - 1; }

private:
    // ---- 价位/索引工具 ----

    [[nodiscard]] HFT_FORCE_INLINE bool price_in_window(Price p) const noexcept {
        return p >= base_ && p <= max_price();
    }
    [[nodiscard]] HFT_FORCE_INLINE Level& level_at(Price p) noexcept {
        return levels_[static_cast<std::size_t>(p - base_)];
    }
    [[nodiscard]] HFT_FORCE_INLINE const Level& level_at(Price p) const noexcept {
        return levels_[static_cast<std::size_t>(p - base_)];
    }
    [[nodiscard]] static constexpr std::size_t bucket_of(OrderId id) noexcept {
        // 64 位斐波那契散列，取高位做桶号（低位雪崩差）。
        return static_cast<std::size_t>(
            (id * 0x9E3779B97F4A7C15ull) >> 32) & (kIndexCap - 1);
    }

    [[nodiscard]] Order* index_find(OrderId id) const noexcept {
        for (Order* o = buckets_[bucket_of(id)]; o != nullptr; o = o->hnext) {
            if (o->id == id) return o;
        }
        return nullptr;
    }
    void index_insert(Order* o) noexcept {
        Order*& b = buckets_[bucket_of(o->id)];
        o->hnext = b;
        b = o;
    }
    void index_erase(Order* o) noexcept {
        Order** pp = &buckets_[bucket_of(o->id)];
        while (*pp != o) pp = &(*pp)->hnext;
        *pp = o->hnext;
    }

    // ---- 驻留/移除 ----

    void rest(Order* o) noexcept {
        Level& lv = level_at(o->price);
        o->prev = lv.tail;
        o->next = nullptr;
        if (lv.tail != nullptr) lv.tail->next = o; else lv.head = o;
        lv.tail = o;
        if (lv.total == 0) bitmap_.set(static_cast<std::size_t>(o->price - base_));
        lv.total += o->qty;

        index_insert(o);
        ++resting_;

        if (o->side == Side::Buy) {
            if (!has_bid_ || o->price > best_bid_) { best_bid_ = o->price; has_bid_ = true; }
        } else {
            if (!has_ask_ || o->price < best_ask_) { best_ask_ = o->price; has_ask_ = true; }
        }
    }

    void unrest(Order* o) noexcept {
        Level& lv = level_at(o->price);
        if (o->prev != nullptr) o->prev->next = o->next; else lv.head = o->next;
        if (o->next != nullptr) o->next->prev = o->prev; else lv.tail = o->prev;
        lv.total -= o->qty;

        if (lv.total == 0) {
            bitmap_.clear(static_cast<std::size_t>(o->price - base_));
            if (o->side == Side::Buy && o->price == best_bid_) rescan_best<Side::Buy>();
            if (o->side == Side::Sell && o->price == best_ask_) rescan_best<Side::Sell>();
        }
        index_erase(o);
        pool_.free(o);
        --resting_;
    }

    template <Side S>
    void rescan_best() noexcept {
        // 位图按"价位占用"记录、不分买卖侧。正常撮合保证同价位不会两侧共存
        // （买单驻留前已扫光该价及更优的卖单），但保守起见仍逐价位校验侧。
        if constexpr (S == Side::Buy) {
            int i = bitmap_.find_last();
            while (i >= 0) {
                if (level_side_nonempty<Side::Buy>(static_cast<std::size_t>(i))) {
                    best_bid_ = base_ + i; has_bid_ = true; return;
                }
                i = bitmap_.find_last_below(static_cast<std::size_t>(i));
            }
            has_bid_ = false;
        } else {
            int i = bitmap_.find_first();
            while (i >= 0) {
                if (level_side_nonempty<Side::Sell>(static_cast<std::size_t>(i))) {
                    best_ask_ = base_ + i; has_ask_ = true; return;
                }
                i = bitmap_.find_first_above(static_cast<std::size_t>(i));
            }
            has_ask_ = false;
        }
    }

    // 位图只记"价位非空"，不记侧。重扫时确认该价位该侧有单。idx 为窗口内下标。
    template <Side S>
    [[nodiscard]] bool level_side_nonempty(std::size_t idx) const noexcept {
        const Level& lv = levels_[idx];
        for (Order* o = lv.head; o != nullptr; o = o->next) {
            if (o->side == S) return true;
        }
        return false;
    }

    // ---- 撮合 ----

    // Opp = 对手方侧。id/price/qty 为吃单方，qty 按引用扣减。
    template <Side Opp, typename Sink>
    void match(OrderId id, Price price, Qty& qty, Sink& on_trade) noexcept {
        while (qty > 0) {
            Price bp; Level* lv;
            if constexpr (Opp == Side::Sell) {
                if (!has_ask_) break;
                bp = best_ask_;
                if (bp > price) break; // 卖一太贵，不再成交
            } else {
                if (!has_bid_) break;
                bp = best_bid_;
                if (bp < price) break; // 买一太低
            }
            lv = &level_at(bp);
            Order* resting_order = lv->head;
            const Qty fill = (qty < resting_order->qty) ? qty : resting_order->qty;

            resting_order->qty -= fill;
            qty -= fill;
            lv->total -= fill;

            on_trade(proto::Trade{
                .aggressor_id = id,
                .resting_id   = resting_order->id,
                .price        = bp,      // 成交价 = 挂单方价格
                .qty          = fill,
            });

            if (resting_order->qty == 0) {
                unrest(resting_order);
            }
        }
    }

    static constexpr std::size_t kIndexCap = MaxOrders * 2; // 负载因子 <= 0.5

    Price base_;
    std::unique_ptr<Level[]>  levels_;
    std::unique_ptr<Order*[]> buckets_;
    LevelBitmap<NumLevels>    bitmap_;
    Pool<Order>               pool_{MaxOrders};

    Price best_bid_ = kInvalidPrice;
    Price best_ask_ = kInvalidPrice;
    bool  has_bid_  = false;
    bool  has_ask_  = false;
    std::size_t resting_ = 0;
};

} // namespace hft
