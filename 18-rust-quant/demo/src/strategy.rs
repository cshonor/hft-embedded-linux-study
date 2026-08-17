//! 做市：同时挂买和卖，别人来成交我们就赚买卖价差。对照 P10 `strategy.hpp`。
//!
//!   PnL ≈ 成交量 × 价差  − 逆向选择  − 库存风险
//!
//! 库存偏斜：手里货多了 → 买价/卖价一起下移，让市场更容易从我们这儿买走。
//! 注意：每 tick 撤旧挂新，在 FIFO 市场会丢掉队列位置。这是教学简化。

use crate::book::OrderBook;
use crate::types::{Order, OrderType, Price, Qty, Side, Trade, INVALID_PRICE, OWNER_US};

#[derive(Clone, Debug)]
pub struct StrategyConfig {
    pub quote_size: Qty,
    /// 中间价上下各挂这么多 tick，这就是我们想赚的价差。
    pub half_spread: Price,
    /// 每囤 `skew_unit` 张，报价整体挪这么多 tick。
    pub skew_per_pos: Price,
    pub skew_unit: Qty,
    pub max_half: Price,
}

impl Default for StrategyConfig {
    fn default() -> Self {
        Self {
            quote_size: 10,
            half_spread: 2,
            skew_per_pos: 1,
            skew_unit: 10,
            max_half: 12,
        }
    }
}

pub struct MarketMaker {
    pub cfg: StrategyConfig,
    /// >0 多头（买多了），<0 空头。
    pub inventory: Qty,
    /// 现金，单位 tick×数量。
    pub cash_ticks: i64,
    pub live_bid_id: u64,
    pub live_ask_id: u64,
    pub fills: i32,
    pub filled_qty: Qty,
    pub last_mid: Price,
    /// 中间价变动的平滑值，用来把价差加宽。不参与撮合，所以这里才允许 `f64`。
    pub vol_ema: f64,
}

impl Default for MarketMaker {
    fn default() -> Self {
        Self {
            cfg: StrategyConfig::default(),
            inventory: 0,
            cash_ticks: 0,
            live_bid_id: 0,
            live_ask_id: 0,
            fills: 0,
            filled_qty: 0,
            last_mid: INVALID_PRICE,
            vol_ema: 0.0,
        }
    }
}

impl MarketMaker {
    pub fn on_fill(&mut self, t: &Trade) {
        let we_taker = t.taker_owner == OWNER_US;
        let we_maker = t.maker_owner == OWNER_US;
        if !we_taker && !we_maker {
            return; // 别人跟别人成交，与我们无关
        }

        // 我们是 taker：方向就是 taker_side。
        // 我们是 maker：对方买则我们在卖，对方卖则我们在买。
        let our_side = if we_taker {
            t.taker_side
        } else if t.taker_side == Side::Buy {
            Side::Sell
        } else {
            Side::Buy
        };

        if our_side == Side::Buy {
            self.inventory += t.qty;
            self.cash_ticks -= t.price * t.qty;
        } else {
            self.inventory -= t.qty;
            self.cash_ticks += t.price * t.qty;
        }
        self.fills += 1;
        self.filled_qty += t.qty;
    }

    /// 把库存按现在的中间价标成现金：账面盈亏。
    pub fn mtm_pnl(&self, mid: Price) -> i64 {
        if mid == INVALID_PRICE {
            self.cash_ticks
        } else {
            self.cash_ticks + self.inventory * mid
        }
    }

    pub fn quote(&mut self, book: &OrderBook, id_seq: &mut u64, ts: u64) -> Vec<Order> {
        if !book.has_bbo() {
            return Vec::new();
        }

        let mid_now = book.mid();
        if self.last_mid != INVALID_PRICE {
            let d = (mid_now - self.last_mid).abs();
            self.vol_ema = 0.9 * self.vol_ema + 0.1 * (d as f64);
        }
        self.last_mid = mid_now;

        let mut half = self.cfg.half_spread + (self.vol_ema + 0.5) as Price;
        if half < self.cfg.half_spread {
            half = self.cfg.half_spread;
        }
        if half > self.cfg.max_half {
            half = self.cfg.max_half;
        }

        let skew = if self.cfg.skew_unit > 0 {
            (self.inventory / self.cfg.skew_unit) * self.cfg.skew_per_pos
        } else {
            0
        };

        // 库存多 → 两边报价下移（更想卖掉）；库存空 → 上移（更想买回）。
        let mut bid_px = mid_now - half - skew;
        let mut ask_px = mid_now + half - skew;
        if bid_px < 1 {
            bid_px = 1;
        }
        if ask_px <= bid_px {
            ask_px = bid_px + 1; // 买价不能超过卖价，否则自己跟自己成交
        }

        *id_seq += 1;
        let bid = Order {
            id: *id_seq,
            owner: OWNER_US,
            side: Side::Buy,
            ty: OrderType::Limit,
            price: bid_px,
            qty: self.cfg.quote_size,
            ts,
        };
        *id_seq += 1;
        let ask = Order {
            id: *id_seq,
            owner: OWNER_US,
            side: Side::Sell,
            ty: OrderType::Limit,
            price: ask_px,
            qty: self.cfg.quote_size,
            ts,
        };
        vec![bid, ask]
    }
}
