//! 行情模拟器：不接真交易所，自己「编」出一个会走的市场。对照 P10 `replay.hpp`。
//!
//! 每个 tick 大致做三件事：
//!   1. 公允价 fair 随机走一步（偶尔大跳 = 新闻/冲击，用来打我们的旧报价）
//!   2. 撤掉并重挂「别人」较宽的买卖盘，保证簿上永远有 BBO
//!   3. 有时丢一笔市价冲击单，先撞我们更窄的报价
//!
//! 跳价 + 冲击 = 逆向选择：你最容易成交的时候，往往是价格已经对你不利。
//!
//! 本 demo **单线程**直接产出 `Vec<Event>`，不手写无锁环（避免 `unsafe`）。
//! 生产里这里会换成 SPSC，见笔记第 7 章。

use crate::types::{Event, Order, OrderType, Price, Qty, Side, OWNER_MARKET};

#[derive(Clone, Debug)]
pub struct ReplayConfig {
    pub ticks: i32,
    /// 起始公允价，100.00 元 = 10000 tick。
    pub start_fair: Price,
    /// 「别人」做市更宽，我们才能挂在里面赚价差。
    pub other_half: Price,
    pub other_size: Qty,
    /// 周期性跳价；`0` 表示关掉，用来对比 PnL。
    pub jump_every: i32,
    pub jump_ticks: Price,
    /// 0.0..=1.0，每个 tick 丢市价冲击的概率。
    pub hit_prob: f64,
    pub hit_max: Qty,
    pub seed: u32,
}

impl Default for ReplayConfig {
    fn default() -> Self {
        Self {
            ticks: 20_000,
            start_fair: 10_000,
            other_half: 4,
            other_size: 40,
            jump_every: 80,
            jump_ticks: 12,
            hit_prob: 0.35,
            hit_max: 12,
            seed: 1,
        }
    }
}

/// xorshift64：零依赖伪随机。不是密码学 RNG，只用来编市场。
struct Rng(u64);

impl Rng {
    fn new(seed: u32) -> Self {
        let mut x = seed as u64;
        if x == 0 {
            x = 0x9E37_79B9;
        }
        Self(x)
    }

    fn next(&mut self) -> u64 {
        let mut x = self.0;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        self.0 = x;
        x
    }

    fn i32_incl(&mut self, lo: i32, hi: i32) -> i32 {
        let span = (hi - lo + 1) as u64;
        lo + (self.next() % span) as i32
    }

    fn coin(&mut self) -> bool {
        self.next() & 1 == 0
    }

    fn chance(&mut self, p: f64) -> bool {
        if p <= 0.0 {
            return false;
        }
        if p >= 1.0 {
            return true;
        }
        let thresh = (p * 10_000.0) as u64;
        self.next() % 10_000 < thresh
    }
}

pub struct Replay {
    pub cfg: ReplayConfig,
    fair: Price,
}

impl Replay {
    pub fn new(cfg: ReplayConfig) -> Self {
        let fair = cfg.start_fair;
        Self { cfg, fair }
    }

    pub fn fair(&self) -> Price {
        self.fair
    }

    /// 生成一整段事件。调用方按顺序 `handle`，语义与 P10 回放线程一致。
    pub fn events(&mut self) -> Vec<Event> {
        let mut rng = Rng::new(self.cfg.seed);
        let mut out = Vec::with_capacity((self.cfg.ticks as usize) * 5);
        let mut id_seq: u64 = 1;
        let mut other_bid: u64 = 0;
        let mut other_ask: u64 = 0;

        for i in 0..self.cfg.ticks {
            self.fair += rng.i32_incl(-1, 1) as Price;
            if self.fair < 1000 {
                self.fair = 1000;
            }
            if self.cfg.jump_every > 0 && (i + 1) % self.cfg.jump_every == 0 {
                let dir: Price = if rng.coin() { 1 } else { -1 };
                self.fair += dir * self.cfg.jump_ticks;
            }

            if other_bid != 0 {
                out.push(Event::cancel(other_bid));
            }
            if other_ask != 0 {
                out.push(Event::cancel(other_ask));
            }

            id_seq += 1;
            other_bid = id_seq;
            id_seq += 1;
            other_ask = id_seq;

            let mut bid = Order::new(
                other_bid,
                OWNER_MARKET,
                Side::Buy,
                OrderType::Limit,
                self.fair - self.cfg.other_half,
                self.cfg.other_size,
            );
            bid.ts = i as u64;
            if bid.price < 1 {
                bid.price = 1;
            }

            let mut ask = Order::new(
                other_ask,
                OWNER_MARKET,
                Side::Sell,
                OrderType::Limit,
                self.fair + self.cfg.other_half,
                self.cfg.other_size,
            );
            ask.ts = i as u64;

            let will_hit = rng.chance(self.cfg.hit_prob);
            out.push(Event::order(bid, false));
            // 没有冲击单时，ask 入簿后就轮到我们报价。
            out.push(Event::order(ask, !will_hit));

            if will_hit {
                id_seq += 1;
                let side = if rng.coin() { Side::Buy } else { Side::Sell };
                let qty = rng.i32_incl(1, self.cfg.hit_max as i32) as Qty;
                let mut hit = Order::new(id_seq, OWNER_MARKET, side, OrderType::Market, 0, qty);
                hit.ts = i as u64;
                out.push(Event::order(hit, true));
            }
        }

        out.push(Event::shutdown());
        out
    }
}
