//! 引擎：按顺序吃 Event，更新订单簿，必要时跑策略+风控+我们的报价。
//! 对照 P10 `engine.hpp`。
//!
//! 一个 tick 的顺序（和实盘「先看行情再下单」一样）：
//!   别人的撤/挂/市价冲击  →  可能打到我们的旧单
//!   然后撤我们旧报价、按新 BBO 挂新买卖
//!   风控不过的单直接扔掉，不进撮合

use std::time::Instant;

use crate::book::OrderBook;
use crate::replay::{Replay, ReplayConfig};
use crate::risk::{RiskGate, RiskReason};
use crate::strategy::MarketMaker;
use crate::types::{Event, EventKind, Price, Qty, Side, Trade, INVALID_PRICE};

#[derive(Clone, Debug, Default)]
pub struct DemoStats {
    pub ticks: i32,
    pub events: i32,
    pub our_fills: i32,
    pub risk_reject: i32,
    pub risk_accept: i32,
    pub inventory: Qty,
    pub pnl: i64,
    pub last_mid: Price,
    pub last_fair: Price,
}

pub struct LatencyHist {
    samples: Vec<u64>,
}

impl LatencyHist {
    fn new() -> Self {
        Self {
            samples: Vec::with_capacity(20_000),
        }
    }

    fn add(&mut self, ns: u64) {
        self.samples.push(ns);
    }

    fn percentile(&self, p: f64) -> u64 {
        if self.samples.is_empty() {
            return 0;
        }
        let mut v = self.samples.clone();
        v.sort_unstable();
        let idx = ((p * (v.len() as f64 - 1.0)).round() as usize).min(v.len() - 1);
        v[idx]
    }

    fn print(&self, label: &str) {
        let n = self.samples.len();
        if n == 0 {
            println!("{label}: (no samples)");
            return;
        }
        println!(
            "{label}: n={n}  p50={}ns  p99={}ns",
            self.percentile(0.50),
            self.percentile(0.99)
        );
    }
}

pub struct Engine {
    pub book: OrderBook,
    pub mm: MarketMaker,
    pub risk: RiskGate,
    pub latency: LatencyHist,
    next_id: u64,
    pub events: i32,
    pub ticks: i32,
}

impl Default for Engine {
    fn default() -> Self {
        Self::new()
    }
}

impl Engine {
    pub fn new() -> Self {
        Self {
            book: OrderBook::new(),
            mm: MarketMaker::default(),
            risk: RiskGate::default(),
            latency: LatencyHist::new(),
            next_id: 1_000_000_000, // 和回放 id（从 1 起）错开，避免撞号
            events: 0,
            ticks: 0,
        }
    }

    fn apply_trades(&mut self, trades: Vec<Trade>) {
        for t in &trades {
            self.mm.on_fill(t);
        }
    }

    pub fn handle(&mut self, e: &Event) {
        self.events += 1;
        match e.kind {
            EventKind::Cancel => {
                let _ = self.book.cancel(e.cancel_id);
            }
            EventKind::Order => {
                let trades = self.book.submit(e.order.clone());
                self.apply_trades(trades);
            }
            EventKind::Shutdown => return,
        }

        if !e.run_strategy {
            return; // 这一笔只是市场更新，还没到我们决策的点
        }

        self.ticks += 1;
        self.risk.on_tick_start();
        let t0 = Instant::now();

        // 每拍先撤旧报价再挂新的（教学用 cancel-replace）。
        if self.mm.live_bid_id != 0 {
            let _ = self.book.cancel(self.mm.live_bid_id);
            self.mm.live_bid_id = 0;
        }
        if self.mm.live_ask_id != 0 {
            let _ = self.book.cancel(self.mm.live_ask_id);
            self.mm.live_ask_id = 0;
        }

        let intents = self
            .mm
            .quote(&self.book, &mut self.next_id, self.ticks as u64);
        for o in intents {
            if self.risk.check(&o, &self.book, self.mm.inventory) != RiskReason::Ok {
                continue;
            }
            let id = o.id;
            let side = o.side;
            let trades = self.book.submit(o);
            self.apply_trades(trades);
            match side {
                Side::Buy => self.mm.live_bid_id = id,
                Side::Sell => self.mm.live_ask_id = id,
            }
        }

        self.latency.add(t0.elapsed().as_nanos() as u64);
    }

    pub fn snapshot(&self, fair: Price) -> DemoStats {
        DemoStats {
            ticks: self.ticks,
            events: self.events,
            our_fills: self.mm.fills,
            risk_reject: self.risk.rejects,
            risk_accept: self.risk.accept,
            inventory: self.mm.inventory,
            last_mid: self.book.mid(),
            pnl: self.mm.mtm_pnl(self.book.mid()),
            last_fair: fair,
        }
    }

    pub fn print_report(&self, fair: Price) {
        let s = self.snapshot(fair);
        let bb = self.book.best_bid();
        let ba = self.book.best_ask();

        println!("\n======== HFT demo report ========");
        println!(
            "ticks={}  events={}  resting={}",
            s.ticks,
            s.events,
            self.book.resting()
        );
        println!(
            "BBO  bid={}.{:02} x {}   ask={}.{:02} x {}   mid={}.{:02}  spread={} tick",
            whole(bb),
            frac(bb),
            self.book.bid_qty(),
            whole(ba),
            frac(ba),
            self.book.ask_qty(),
            whole(s.last_mid),
            frac(s.last_mid),
            self.book.spread()
        );
        println!(
            "fair={}.{:02}  inventory={}  cash_ticks={}  mtm_PnL={} ticks ({:.2})",
            whole(s.last_fair),
            frac(s.last_fair),
            s.inventory,
            self.mm.cash_ticks,
            s.pnl,
            s.pnl as f64 / 100.0
        );
        println!(
            "our fills={}  filled_qty={}  risk accept={}  reject={}",
            s.our_fills, self.mm.filled_qty, s.risk_accept, s.risk_reject
        );
        self.latency.print("compute  (quote+risk+submit)");
        println!("=================================");
        println!("PnL 单位：1.00 = 100 tick。正数 ≈ 赚到价差；跳价密集时库存会被打偏。");
        println!("本 demo 单线程，没有 SPSC 排队延迟；对照 P10 的 queue_wait。");
    }
}

fn whole(px: Price) -> i64 {
    if px == INVALID_PRICE {
        0
    } else {
        px / 100
    }
}

fn frac(px: Price) -> i64 {
    if px == INVALID_PRICE {
        0
    } else {
        px.abs() % 100
    }
}

/// 跑完整回放。`print=false` 给单元测试用。
pub fn run_demo(cfg: ReplayConfig, print: bool) -> DemoStats {
    let mut replay = Replay::new(cfg);
    let events = replay.events();
    let fair = replay.fair();
    let mut engine = Engine::new();
    for e in &events {
        if e.kind == EventKind::Shutdown {
            break;
        }
        engine.handle(e);
    }
    if print {
        engine.print_report(fair);
    }
    engine.snapshot(fair)
}
