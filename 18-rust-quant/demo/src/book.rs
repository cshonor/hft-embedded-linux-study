//! 限价订单簿（LOB）= 买单簿 + 卖单簿。对照 P10 `orderbook.hpp`。
//!
//!   买单 bids：价格从高到低 —— 最优买 = 最大键
//!   卖单 asks：价格从低到高 —— 最优卖 = 最小键
//!
//! 同一价格上多张单用 `VecDeque` 排队：先到先成交（FIFO）。
//!
//! 撮合三种结果：
//!   Best Price   买单先吃最便宜的卖
//!   Partial Fill 只吃到一部分，限价剩余再挂上
//!   No Match     价格碰不上，整单挂上
//!
//! 成交价 = 被吃那张挂单的价格。demo 用 `BTreeMap`，正确性优先。

use std::collections::{BTreeMap, HashMap, VecDeque};

use crate::types::{
    Order, OrderType, Price, Qty, Side, Trade, INVALID_PRICE, MAX_PRICE, OWNER_MARKET,
};

struct PriceLevel {
    orders: VecDeque<Order>,
    total: Qty,
}

struct BookLoc {
    side: Side,
    price: Price,
}

/// 限价订单簿。字段不对外可变：撮合必须走 `submit` / `cancel`，否则 FIFO 索引会坏。
pub struct OrderBook {
    bids: BTreeMap<Price, PriceLevel>,
    asks: BTreeMap<Price, PriceLevel>,
    index: HashMap<u64, BookLoc>,
}

impl Default for OrderBook {
    fn default() -> Self {
        Self::new()
    }
}

impl OrderBook {
    pub fn new() -> Self {
        Self {
            bids: BTreeMap::new(),
            asks: BTreeMap::new(),
            index: HashMap::new(),
        }
    }

    pub fn best_bid(&self) -> Price {
        self.bids
            .keys()
            .next_back()
            .copied()
            .unwrap_or(INVALID_PRICE)
    }

    pub fn best_ask(&self) -> Price {
        self.asks.keys().next().copied().unwrap_or(INVALID_PRICE)
    }

    pub fn has_bbo(&self) -> bool {
        !self.bids.is_empty() && !self.asks.is_empty()
    }

    pub fn mid(&self) -> Price {
        if !self.has_bbo() {
            return INVALID_PRICE;
        }
        (self.best_bid() + self.best_ask()) / 2
    }

    pub fn spread(&self) -> Price {
        if !self.has_bbo() {
            return INVALID_PRICE;
        }
        self.best_ask() - self.best_bid()
    }

    pub fn bid_qty(&self) -> Qty {
        self.bids
            .values()
            .next_back()
            .map(|lvl| lvl.total)
            .unwrap_or(0)
    }

    pub fn ask_qty(&self) -> Qty {
        self.asks.values().next().map(|lvl| lvl.total).unwrap_or(0)
    }

    pub fn resting(&self) -> usize {
        self.index.len()
    }

    pub fn submit(&mut self, mut order: Order) -> Vec<Trade> {
        if order.qty <= 0 {
            return Vec::new();
        }

        // 市价单不看价格：买单当成「无限高」，卖单当成 0，好走同一套 crosses()。
        if order.ty == OrderType::Market {
            order.price = if order.side == Side::Buy {
                MAX_PRICE
            } else {
                0
            };
        }

        // FOK：先问「对手盘够不够」，不够整单作废，连一股都不成交。
        if order.ty == OrderType::Fok {
            if self.available_to_fill(&order) < order.qty {
                return Vec::new();
            }
            order.ty = OrderType::Ioc;
        }

        let mut trades = Vec::new();
        self.match_incoming(&mut order, &mut trades);

        // Limit 才挂剩余；Market / IOC 吃完就停，不排队。
        if order.ty == OrderType::Limit && order.qty > 0 {
            self.rest_order(order);
        }
        trades
    }

    pub fn cancel(&mut self, id: u64) -> bool {
        let Some(loc) = self.index.remove(&id) else {
            return false;
        };
        let map = match loc.side {
            Side::Buy => &mut self.bids,
            Side::Sell => &mut self.asks,
        };
        let Some(lvl) = map.get_mut(&loc.price) else {
            return false;
        };
        if let Some(pos) = lvl.orders.iter().position(|o| o.id == id) {
            let Some(removed) = lvl.orders.remove(pos) else {
                return false;
            };
            lvl.total -= removed.qty;
            if lvl.orders.is_empty() {
                map.remove(&loc.price);
            }
            return true;
        }
        false
    }

    pub fn available_to_fill(&self, o: &Order) -> Qty {
        let mut avail = 0;
        if o.side == Side::Buy {
            for (&px, lvl) in &self.asks {
                if o.ty != OrderType::Market && o.price < px {
                    break;
                }
                avail += qty_from_others(lvl, o.owner);
                if avail >= o.qty {
                    return o.qty;
                }
            }
        } else {
            for (&px, lvl) in self.bids.iter().rev() {
                if o.ty != OrderType::Market && o.price > px {
                    break;
                }
                avail += qty_from_others(lvl, o.owner);
                if avail >= o.qty {
                    return o.qty;
                }
            }
        }
        avail
    }

    fn match_incoming(&mut self, incoming: &mut Order, trades: &mut Vec<Trade>) {
        if incoming.side == Side::Buy {
            // 买单吃卖盘：从最便宜的 ask 往上走。先拷贝键，避免一边遍历一边改 map。
            let keys: Vec<Price> = self.asks.keys().copied().collect();
            for px in keys {
                if incoming.qty <= 0 {
                    break;
                }
                if !crosses(incoming, px) {
                    break;
                }
                match_level(&mut self.asks, &mut self.index, px, incoming, trades);
            }
        } else {
            let keys: Vec<Price> = self.bids.keys().copied().rev().collect();
            for px in keys {
                if incoming.qty <= 0 {
                    break;
                }
                if !crosses(incoming, px) {
                    break;
                }
                match_level(&mut self.bids, &mut self.index, px, incoming, trades);
            }
        }
    }

    fn rest_order(&mut self, o: Order) {
        // push_back = 排到该价位队尾，保证时间优先。
        let loc = BookLoc {
            side: o.side,
            price: o.price,
        };
        let map = match o.side {
            Side::Buy => &mut self.bids,
            Side::Sell => &mut self.asks,
        };
        let lvl = map.entry(o.price).or_insert_with(|| PriceLevel {
            orders: VecDeque::new(),
            total: 0,
        });
        lvl.total += o.qty;
        self.index.insert(o.id, loc);
        lvl.orders.push_back(o);
    }
}

fn qty_from_others(lvl: &PriceLevel, owner: u32) -> Qty {
    let mut q = 0;
    for o in &lvl.orders {
        // 市场侧 owner=0 表示「许多人」，彼此可以成交；只对自己的策略单做 STP。
        if owner != OWNER_MARKET && o.owner == owner {
            continue;
        }
        q += o.qty;
    }
    q
}

fn crosses(o: &Order, opp: Price) -> bool {
    if o.side == Side::Buy {
        o.price >= opp
    } else {
        o.price <= opp
    }
}

fn match_level(
    opp: &mut BTreeMap<Price, PriceLevel>,
    index: &mut HashMap<u64, BookLoc>,
    px: Price,
    incoming: &mut Order,
    trades: &mut Vec<Trade>,
) {
    let Some(lvl) = opp.get_mut(&px) else {
        return;
    };
    let mut i = 0;
    while incoming.qty > 0 && i < lvl.orders.len() {
        if incoming.owner != OWNER_MARKET && lvl.orders[i].owner == incoming.owner {
            i += 1; // 自成交保护：跳过自己的挂单，继续找别人
            continue;
        }
        let q = incoming.qty.min(lvl.orders[i].qty);
        trades.push(Trade {
            taker_id: incoming.id,
            maker_id: lvl.orders[i].id,
            taker_owner: incoming.owner,
            maker_owner: lvl.orders[i].owner,
            taker_side: incoming.side,
            price: px,
            qty: q,
        });
        incoming.qty -= q;
        lvl.orders[i].qty -= q;
        lvl.total -= q;
        if lvl.orders[i].qty == 0 {
            let id = lvl.orders[i].id;
            lvl.orders.remove(i);
            index.remove(&id);
        } else {
            i += 1;
        }
    }
    if lvl.orders.is_empty() {
        opp.remove(&px);
    }
}
