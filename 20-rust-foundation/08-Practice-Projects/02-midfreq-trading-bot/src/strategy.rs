use crate::market::Bar;

pub trait Strategy: Send {
    fn on_bar(&mut self, bar: &Bar) -> Option<Signal>;
}

#[derive(Debug, Clone, Copy)]
pub enum Signal {
    Buy { qty: f64 },
    Sell { qty: f64 },
}

/// Simple two-bar momentum placeholder.
pub struct DemoMomentum;

impl Strategy for DemoMomentum {
    fn on_bar(&mut self, bar: &Bar) -> Option<Signal> {
        if bar.close > bar.open {
            Some(Signal::Buy { qty: 1.0 })
        } else {
            None
        }
    }
}
