use crate::market::Bar;

pub trait Strategy: Send {
    fn on_bar(&mut self, bar: &Bar) -> Option<Signal>;
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum Signal {
    Buy { qty: f64 },
    Sell { qty: f64 },
}

impl Signal {
    pub fn qty(&self) -> f64 {
        match self {
            Signal::Buy { qty } | Signal::Sell { qty } => *qty,
        }
    }

    /// 该信号对持仓的方向：买入 +1，卖出 -1。
    pub fn signed_qty(&self) -> f64 {
        match self {
            Signal::Buy { qty } => *qty,
            Signal::Sell { qty } => -*qty,
        }
    }
}

/// 两 bar 动量：与上一根收盘价比较决定方向。
///
/// 维护 `position`，保证：
/// - 首根 bar 无参照 → 不出信号
/// - 空仓时上涨 → `Buy`；已持多仓再涨 → 不出信号（不重复加仓）
/// - 持多仓时下跌 → `Sell` 平仓；空仓再跌 → 不出信号（不做空）
pub struct DemoMomentum {
    prev_close: Option<f64>,
    position: f64,
    qty: f64,
}

impl DemoMomentum {
    pub fn new(qty: f64) -> Self {
        Self {
            prev_close: None,
            position: 0.0,
            qty,
        }
    }

    pub fn position(&self) -> f64 {
        self.position
    }
}

impl Strategy for DemoMomentum {
    fn on_bar(&mut self, bar: &Bar) -> Option<Signal> {
        // 注意：必须先落盘 prev_close 再返回，否则首根 bar 之后 prev_close 永远是 None，
        // 策略从此再也不出信号（这个 bug 就是被下面的单测抓出来的）。
        let prev = match self.prev_close {
            Some(p) => p,
            None => {
                self.prev_close = Some(bar.close);
                return None;
            }
        };
        self.prev_close = Some(bar.close);

        if bar.close > prev && self.position <= 0.0 {
            self.position += self.qty;
            Some(Signal::Buy { qty: self.qty })
        } else if bar.close < prev && self.position > 0.0 {
            let qty = self.position.min(self.qty);
            self.position -= qty;
            Some(Signal::Sell { qty })
        } else {
            None
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn bar(close: f64) -> Bar {
        Bar {
            symbol: "DEMO",
            open: close,
            high: close + 1.0,
            low: close - 1.0,
            close,
            volume: 1_000.0,
        }
    }

    #[test]
    fn first_bar_yields_no_signal() {
        let mut s = DemoMomentum::new(1.0);
        assert!(s.on_bar(&bar(100.0)).is_none());
    }

    #[test]
    fn up_move_opens_long_then_down_move_closes_it() {
        let mut s = DemoMomentum::new(1.0);
        s.on_bar(&bar(100.0)); // 建立参照

        assert_eq!(s.on_bar(&bar(101.0)), Some(Signal::Buy { qty: 1.0 }));
        assert_eq!(s.position(), 1.0);

        // 已持多仓，继续上涨不重复开仓
        assert!(s.on_bar(&bar(102.0)).is_none());

        // 下跌平仓
        assert_eq!(s.on_bar(&bar(101.0)), Some(Signal::Sell { qty: 1.0 }));
        assert_eq!(s.position(), 0.0);

        // 空仓下跌不做空
        assert!(s.on_bar(&bar(100.0)).is_none());
    }
}
