//! 本地风控：策略再怎么 bug，单子也先过这一关才进撮合。对照 P10 `risk.hpp`。
//! 热路径只做几次 `if`，不分配、不加锁、不调系统调用。

use crate::book::OrderBook;
use crate::types::{Order, OrderType, Price, Qty, Side, INVALID_PRICE};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum RiskReason {
    Ok,
    KillSwitch,
    QtyCap,
    PriceBand,
    PositionCap,
    RateLimit,
}

#[derive(Clone, Debug)]
pub struct RiskConfig {
    /// 报价不能偏离 mid 超过这么多 tick。
    pub band_ticks: Price,
    pub max_order_qty: Qty,
    pub max_position: Qty,
    pub max_orders_per_tick: i32,
}

impl Default for RiskConfig {
    fn default() -> Self {
        Self {
            band_ticks: 80,
            max_order_qty: 50,
            max_position: 150,
            max_orders_per_tick: 4,
        }
    }
}

pub struct RiskGate {
    pub cfg: RiskConfig,
    pub kill: bool,
    pub orders_this_tick: i32,
    pub rejects: i32,
    pub accept: i32,
}

impl Default for RiskGate {
    fn default() -> Self {
        Self {
            cfg: RiskConfig::default(),
            kill: false,
            orders_this_tick: 0,
            rejects: 0,
            accept: 0,
        }
    }
}

impl RiskGate {
    pub fn on_tick_start(&mut self) {
        self.orders_this_tick = 0;
    }

    pub fn check(&mut self, o: &Order, book: &OrderBook, inventory: Qty) -> RiskReason {
        if self.kill {
            self.rejects += 1;
            return RiskReason::KillSwitch;
        }
        if o.qty <= 0 || o.qty > self.cfg.max_order_qty {
            self.rejects += 1;
            return RiskReason::QtyCap;
        }
        if self.orders_this_tick >= self.cfg.max_orders_per_tick {
            self.rejects += 1;
            return RiskReason::RateLimit;
        }

        let mid = book.mid();
        if o.ty == OrderType::Limit && mid != INVALID_PRICE {
            let dist = (o.price - mid).abs();
            if dist > self.cfg.band_ticks {
                self.rejects += 1;
                return RiskReason::PriceBand;
            }
        }

        let next = if o.side == Side::Buy {
            inventory + o.qty
        } else {
            inventory - o.qty
        };
        let abs_next = next.abs();
        let abs_now = inventory.abs();
        // 已经超限时，只许减仓（绝对仓位变小），不许再加。
        if abs_next > self.cfg.max_position && abs_next > abs_now {
            self.rejects += 1;
            return RiskReason::PositionCap;
        }

        self.orders_this_tick += 1;
        self.accept += 1;
        RiskReason::Ok
    }
}
