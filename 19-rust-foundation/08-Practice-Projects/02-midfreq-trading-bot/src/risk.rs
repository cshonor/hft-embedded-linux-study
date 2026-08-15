use crate::strategy::Signal;

pub struct RiskGate {
    pub max_qty: f64,
}

impl RiskGate {
    pub fn new(max_qty: f64) -> Self {
        Self { max_qty }
    }

    pub fn check(&self, signal: &Signal) -> bool {
        match signal {
            Signal::Buy { qty } | Signal::Sell { qty } => *qty <= self.max_qty,
        }
    }
}
