//! Normalized OHLCV bar (mid-low frequency).

#[derive(Debug, Clone, Copy)]
pub struct Bar {
    pub symbol: &'static str,
    pub open: f64,
    pub high: f64,
    pub low: f64,
    pub close: f64,
    pub volume: f64,
}

impl Bar {
    /// 收盘 > 开盘 → 阳线。动量策略最朴素的方向判据。
    pub fn is_bullish(&self) -> bool {
        self.close > self.open
    }

    /// 当日振幅（high − low），用于波动过滤。
    pub fn range(&self) -> f64 {
        self.high - self.low
    }

    /// 成交额（close × volume），用于流动性过滤。
    pub fn notional(&self) -> f64 {
        self.close * self.volume
    }
}

pub struct MockFeed;

impl MockFeed {
    /// 7 根 bar，刻意走完「无参照 → 开多 → 持有 → 平仓 → 空仓 → 再开多 → 再平仓」，
    /// 让 `Signal::Buy` 与 `Signal::Sell` 两条路径都被真正执行到，且**回到空仓**收尾
    ///（原来只有 2 根 bar，Sell 分支永远走不到，等于没实现卖出）。
    pub fn sample_bars() -> Vec<Bar> {
        let closes = [100.5, 101.5, 102.5, 101.0, 100.0, 100.8, 100.2];
        let mut bars = Vec::with_capacity(closes.len());
        let mut prev_close = 100.0;

        for close in closes {
            let open = prev_close;
            bars.push(Bar {
                symbol: "DEMO",
                open,
                high: open.max(close) + 0.5,
                low: open.min(close) - 0.5,
                close,
                volume: if close >= open { 1_200.0 } else { 900.0 },
            });
            prev_close = close;
        }
        bars
    }
}
