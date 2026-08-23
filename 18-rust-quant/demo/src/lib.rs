//! P10 C++ demo 的 Rust 对照：订单簿 + 做市 + 风控。
//!
//! 语法细节回 [17-rust-foundation](../../17-rust-foundation/)。
//! 工程骨架回 [14-hft-engineering](../../14-hft-engineering/)。
//! 本 crate **不是实盘**。

pub mod book;
pub mod engine;
pub mod replay;
pub mod risk;
pub mod strategy;
pub mod types;

pub use book::OrderBook;
pub use engine::{run_demo, DemoStats, Engine};
pub use replay::ReplayConfig;
pub use risk::{RiskGate, RiskReason};
pub use strategy::MarketMaker;
pub use types::{Order, OrderType, Side, Trade, INVALID_PRICE, OWNER_MARKET, OWNER_US};

#[cfg(test)]
mod tests {
    use super::*;
    use crate::types::Price;

    fn mk(id: &mut u64, side: Side, px: Price, qty: i64, ty: OrderType, owner: u32) -> Order {
        *id += 1;
        Order::new(*id, owner, side, ty, px, qty)
    }

    fn mk_mkt(id: &mut u64, side: Side, px: Price, qty: i64) -> Order {
        mk(id, side, px, qty, OrderType::Limit, OWNER_MARKET)
    }

    #[test]
    fn best_price() {
        // 买 105，簿上卖 103 与 104 → 先 103
        let mut id = 1;
        let mut b = OrderBook::new();
        b.submit(mk_mkt(&mut id, Side::Sell, 10400, 50));
        b.submit(mk_mkt(&mut id, Side::Sell, 10300, 200));
        let t = b.submit(mk_mkt(&mut id, Side::Buy, 10500, 100));
        assert_eq!(t.len(), 1);
        assert_eq!(t[0].price, 10300);
        assert_eq!(t[0].qty, 100);
    }

    #[test]
    fn partial_fill() {
        let mut id = 1;
        let mut b = OrderBook::new();
        b.submit(mk_mkt(&mut id, Side::Sell, 10000, 1));
        let t = b.submit(mk_mkt(&mut id, Side::Buy, 10000, 4));
        assert_eq!(t.len(), 1);
        assert_eq!(t[0].qty, 1);
        assert_eq!(b.best_bid(), 10000);
        assert_eq!(b.bid_qty(), 3);
    }

    #[test]
    fn no_match() {
        let mut id = 1;
        let mut b = OrderBook::new();
        b.submit(mk_mkt(&mut id, Side::Sell, 9900, 10));
        let t = b.submit(mk_mkt(&mut id, Side::Buy, 9800, 10));
        assert!(t.is_empty());
        assert_eq!(b.best_bid(), 9800);
        assert_eq!(b.best_ask(), 9900);
    }

    #[test]
    fn fifo() {
        let mut id = 1;
        let mut b = OrderBook::new();
        let a = mk_mkt(&mut id, Side::Sell, 10000, 10);
        let a_id = a.id;
        b.submit(a);
        b.submit(mk_mkt(&mut id, Side::Sell, 10000, 10));
        let t = b.submit(mk_mkt(&mut id, Side::Buy, 10000, 10));
        assert_eq!(t.len(), 1);
        assert_eq!(t[0].maker_id, a_id);
    }

    #[test]
    fn cancel_removes_only_ask() {
        let mut id = 1;
        let mut b = OrderBook::new();
        let a = mk_mkt(&mut id, Side::Sell, 10000, 10);
        let a_id = a.id;
        b.submit(a);
        assert!(b.cancel(a_id));
        assert_eq!(b.best_ask(), INVALID_PRICE);
    }

    #[test]
    fn market_walks_two_levels() {
        let mut id = 1;
        let mut b = OrderBook::new();
        b.submit(mk_mkt(&mut id, Side::Sell, 10000, 5));
        b.submit(mk_mkt(&mut id, Side::Sell, 10100, 5));
        let t = b.submit(mk(
            &mut id,
            Side::Buy,
            0,
            8,
            OrderType::Market,
            OWNER_MARKET,
        ));
        let q: i64 = t.iter().map(|x| x.qty).sum();
        assert_eq!(q, 8);
        assert_eq!(t[0].price, 10000);
        assert_eq!(t[1].price, 10100);
    }

    #[test]
    fn ioc_remainder_does_not_rest() {
        let mut id = 1;
        let mut b = OrderBook::new();
        b.submit(mk_mkt(&mut id, Side::Sell, 10000, 3));
        let t = b.submit(mk(
            &mut id,
            Side::Buy,
            10000,
            10,
            OrderType::Ioc,
            OWNER_MARKET,
        ));
        assert_eq!(t.len(), 1);
        assert_eq!(t[0].qty, 3);
        assert!(!b.has_bbo());
    }

    #[test]
    fn fok_rejects_when_size_insufficient() {
        let mut id = 1;
        let mut b = OrderBook::new();
        b.submit(mk_mkt(&mut id, Side::Sell, 10000, 3));
        let t = b.submit(mk(
            &mut id,
            Side::Buy,
            10000,
            10,
            OrderType::Fok,
            OWNER_MARKET,
        ));
        assert!(t.is_empty());
        assert_eq!(b.best_ask(), 10000);
    }

    #[test]
    fn stp_same_owner_must_not_match() {
        let mut id = 1;
        let mut b = OrderBook::new();
        b.submit(mk(
            &mut id,
            Side::Sell,
            10000,
            10,
            OrderType::Limit,
            OWNER_US,
        ));
        let t = b.submit(mk(
            &mut id,
            Side::Buy,
            10000,
            10,
            OrderType::Limit,
            OWNER_US,
        ));
        assert!(t.is_empty());
        assert_eq!(b.best_bid(), 10000);
        assert_eq!(b.best_ask(), 10000);
    }

    #[test]
    fn risk_band() {
        let mut id = 1;
        let mut b = OrderBook::new();
        b.submit(mk_mkt(&mut id, Side::Buy, 10000, 10));
        b.submit(mk_mkt(&mut id, Side::Sell, 10020, 10));
        let mut g = RiskGate::default();
        g.cfg.band_ticks = 5;
        let far = mk(&mut id, Side::Buy, 10100, 1, OrderType::Limit, OWNER_US);
        assert_eq!(g.check(&far, &b, 0), RiskReason::PriceBand);
    }

    #[test]
    fn risk_pos_add_rejected_reduce_ok() {
        let mut id = 1;
        let mut b = OrderBook::new();
        b.submit(mk_mkt(&mut id, Side::Buy, 10000, 10));
        b.submit(mk_mkt(&mut id, Side::Sell, 10020, 10));
        let mut g = RiskGate::default();
        g.cfg.max_position = 10;
        let buy = mk(&mut id, Side::Buy, 10000, 10, OrderType::Limit, OWNER_US);
        assert_eq!(g.check(&buy, &b, 10), RiskReason::PositionCap);
        g.on_tick_start();
        let sell = mk(&mut id, Side::Sell, 10020, 10, OrderType::Limit, OWNER_US);
        assert_eq!(g.check(&sell, &b, 10), RiskReason::Ok);
    }

    #[test]
    fn maker_inventory_skew_and_fill() {
        // mid = (10000+10020)/2 = 10010；默认 half=2、库存 0 → 买 10008 / 卖 10012
        let mut id = 1;
        let mut b = OrderBook::new();
        b.submit(mk_mkt(&mut id, Side::Buy, 10000, 10));
        b.submit(mk_mkt(&mut id, Side::Sell, 10020, 10));

        let mut mm = MarketMaker::default();
        let quotes = mm.quote(&b, &mut id, 0);
        assert_eq!(quotes.len(), 2);
        assert_eq!(quotes[0].side, Side::Buy);
        assert_eq!(quotes[0].price, 10008);
        assert_eq!(quotes[1].side, Side::Sell);
        assert_eq!(quotes[1].price, 10012);

        // 库存 20、skew_unit=10 → 整体下移 2 tick
        mm.inventory = 20;
        let skewed = mm.quote(&b, &mut id, 0);
        assert_eq!(skewed[0].price, 10006);
        assert_eq!(skewed[1].price, 10010);

        // 别人吃我们的卖：我们是 maker、对方 Buy → 我们在卖，库存下降、收现金
        mm.inventory = 0;
        mm.on_fill(&Trade {
            taker_id: 99,
            maker_id: quotes[1].id,
            taker_owner: OWNER_MARKET,
            maker_owner: OWNER_US,
            taker_side: Side::Buy,
            price: 10012,
            qty: 10,
        });
        assert_eq!(mm.inventory, -10);
        assert_eq!(mm.cash_ticks, 10012 * 10);
        assert_eq!(mm.mtm_pnl(10010), 10012 * 10 + (-10) * 10010);
    }

    #[test]
    fn risk_then_book_like_engine_tick() {
        // 最小引擎拍：风控过了才 submit；别人来吃我们的卖，库存跟着变。
        let mut id = 1;
        let mut b = OrderBook::new();
        b.submit(mk_mkt(&mut id, Side::Buy, 10000, 10));
        b.submit(mk_mkt(&mut id, Side::Sell, 10020, 10));

        let mut mm = MarketMaker::default();
        let mut risk = RiskGate::default();
        risk.on_tick_start();
        for o in mm.quote(&b, &mut id, 0) {
            assert_eq!(risk.check(&o, &b, mm.inventory), RiskReason::Ok);
            for t in b.submit(o) {
                mm.on_fill(&t);
            }
        }
        assert_eq!(b.resting(), 4); // 市场两边 + 我们两边

        let hit = mk_mkt(&mut id, Side::Buy, 10012, 10);
        for t in b.submit(hit) {
            mm.on_fill(&t);
        }
        assert_eq!(mm.inventory, -10);
        assert_eq!(mm.fills, 1);
    }

    #[test]
    fn replay_pipeline_runs() {
        let mut cfg = ReplayConfig::default();
        cfg.ticks = 300;
        let stats = run_demo(cfg, false);
        assert_eq!(stats.ticks, 300);
        assert!(stats.events > 300);
        assert!(stats.last_mid != INVALID_PRICE);
    }
}
