use crate::market::MockFeed;
use crate::oms::Oms;
use crate::risk::RiskGate;
use crate::strategy::{DemoMomentum, Strategy};

pub async fn run_demo() {
    let mut strategy = DemoMomentum;
    let risk = RiskGate::new(10.0);
    let oms = Oms;

    for bar in MockFeed::sample_bars() {
        tracing::info!(?bar, "tick");
        if let Some(signal) = strategy.on_bar(&bar) {
            if risk.check(&signal) {
                let order = oms.submit(signal);
                tracing::info!(?order, "order accepted");
            } else {
                tracing::warn!("risk rejected signal");
            }
        }
    }
}
