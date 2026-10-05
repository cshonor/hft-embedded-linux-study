//! LOB 订单簿：价格优先 + FIFO（P8 Phase 1+2 的 Rust 版）
//!
//! 数据结构：`BTreeMap<Price, Level>` 双簿——
//!   asks 升序，最优卖 = `iter().next()`
//!   bids 升序，最优买 = `iter().next_back()`
//! 价位内 `VecDeque<Order>` 保 FIFO。
//!
//! 安全边界：**零 unsafe**（18-rust-quant 纪律：unsafe 只允许出现在无锁队列，
//! 本阶段还没有无锁队列，所以整个 crate 不需要它）。

use std::collections::{BTreeMap, VecDeque};

use crate::types::{Order, OrderType, Price, Qty, Side, Trade};

/// 一个价位：FIFO 队列
#[derive(Debug, Default)]
struct Level {
    q: VecDeque<Order>,
}

/// 限价订单簿 = 买单簿 + 卖单簿
#[derive(Debug, Default)]
pub struct Book {
    bids: BTreeMap<Price, Level>,
    asks: BTreeMap<Price, Level>,
}

impl Book {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn best_bid(&self) -> Option<Price> {
        self.bids.keys().next_back().copied()
    }

    pub fn best_ask(&self) -> Option<Price> {
        self.asks.keys().next().copied()
    }

    /// 下单入口：按订单类型分发（Phase 2）
    pub fn submit(&mut self, order: Order) -> Vec<Trade> {
        match order.otype {
            OrderType::Limit => {
                let (trades, rest) = self.match_taker(order);
                if let Some(rest) = rest {
                    self.rest(rest);
                }
                trades
            }
            OrderType::Market => {
                // 市价：不检查价格，吃完或吃净对手簿；剩余不挂簿
                self.match_taker(order).0
            }
            OrderType::Ioc => {
                // IOC：限价撮合，剩余立即撤销（不挂簿）
                self.match_taker(order).0
            }
            OrderType::Fok => {
                // FOK：预扫描可成交量，不够全部撤销（一笔都不成交）
                if self.available(order.side, order.price) < order.qty {
                    return Vec::new();
                }
                self.match_taker(order).0
            }
        }
    }

    /// 撤单（两簿各扫一遍；生产版应加 id→价位 索引，见 part-a C++ 同款 TODO）
    pub fn cancel(&mut self, id: u64) -> bool {
        fn drop_from(book: &mut BTreeMap<Price, Level>, id: u64) -> bool {
            let mut found_at = None;
            for (&price, level) in book.iter_mut() {
                if let Some(pos) = level.q.iter().position(|o| o.id == id) {
                    level.q.remove(pos);
                    found_at = Some(price);
                    break;
                }
            }
            let Some(price) = found_at else { return false };
            // 删空了的价位要从簿上摘掉（找到≠删空：同价位还有其他单时价位保留）
            if book.get(&price).is_some_and(|l| l.q.is_empty()) {
                book.remove(&price);
            }
            true
        }
        drop_from(&mut self.bids, id) || drop_from(&mut self.asks, id)
    }

    // ---------- 内部 ----------

    /// 核心撮合：taker 吃对手簿，返回 (成交列表, 未成交残余)
    ///
    /// 限价检查只对 Limit/Ioc/Fok 生效；Market 视为无价格边界。
    fn match_taker(&mut self, taker: Order) -> (Vec<Trade>, Option<Order>) {
        let mut taker = taker;
        let mut trades = Vec::new();
        match taker.side {
            Side::Buy => Self::match_side(
                &mut self.asks, &mut taker, &mut trades,
                |ask_price, t| t.otype == OrderType::Market || t.price >= ask_price,
            ),
            Side::Sell => Self::match_side_rev(
                &mut self.bids, &mut taker, &mut trades,
                |bid_price, t| t.otype == OrderType::Market || t.price <= bid_price,
            ),
        }
        let rest = if taker.qty > 0 { Some(taker) } else { None };
        (trades, rest)
    }

    /// 吃升序簿（asks，最优 = 最小价）：`crossed(price, taker)` 判定是否还能成交
    fn match_side(
        opp: &mut BTreeMap<Price, Level>,
        taker: &mut Order,
        trades: &mut Vec<Trade>,
        crossed: impl Fn(Price, &Order) -> bool,
    ) {
        while taker.qty > 0 {
            let Some((&price, _)) = opp.iter().next() else { break };
            if !crossed(price, taker) {
                break;
            }
            let level = opp.get_mut(&price).expect("level exists");
            Self::fill_level(level, price, taker, trades);
            if level.q.is_empty() {
                opp.remove(&price);
            }
        }
    }

    /// 吃降序侧（bids，最优 = 最大价）：bids 本身升序存，取 next_back
    fn match_side_rev(
        opp: &mut BTreeMap<Price, Level>,
        taker: &mut Order,
        trades: &mut Vec<Trade>,
        crossed: impl Fn(Price, &Order) -> bool,
    ) {
        while taker.qty > 0 {
            let Some((&price, _)) = opp.iter().next_back() else { break };
            if !crossed(price, taker) {
                break;
            }
            let level = opp.get_mut(&price).expect("level exists");
            Self::fill_level(level, price, taker, trades);
            if level.q.is_empty() {
                opp.remove(&price);
            }
        }
    }

    /// 在一个价位内按 FIFO 撮合；成交价 = 该价位（maker 价）
    fn fill_level(level: &mut Level, price: Price, taker: &mut Order, trades: &mut Vec<Trade>) {
        while taker.qty > 0 {
            let Some(maker) = level.q.front_mut() else { break };
            let qn = taker.qty.min(maker.qty);
            trades.push(Trade { taker_id: taker.id, maker_id: maker.id, price, qty: qn });
            taker.qty -= qn;
            maker.qty -= qn;
            if maker.qty == 0 {
                level.q.pop_front();
            }
        }
    }

    /// 残余量挂簿
    fn rest(&mut self, order: Order) {
        let book = match order.side {
            Side::Buy => &mut self.bids,
            Side::Sell => &mut self.asks,
        };
        book.entry(order.price).or_default().q.push_back(order);
    }

    /// FOK 预扫描：对手簿在价格边界内的可成交总量（只读）
    fn available(&self, side: Side, limit: Price) -> Qty {
        match side {
            Side::Buy => self
                .asks
                .iter()
                .take_while(|(&p, _)| p <= limit)
                .map(|(_, l)| l.q.iter().map(|o| o.qty).sum::<Qty>())
                .sum(),
            Side::Sell => self
                .bids
                .iter()
                .rev()
                .take_while(|(&p, _)| p >= limit)
                .map(|(_, l)| l.q.iter().map(|o| o.qty).sum::<Qty>())
                .sum(),
        }
    }
}
