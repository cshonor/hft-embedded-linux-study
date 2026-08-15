use crate::strategy::Signal;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum OrderState {
    New,
    Accepted,
}

#[derive(Debug)]
pub struct Order {
    pub state: OrderState,
    pub signal: Signal,
}

pub struct Oms;

impl Oms {
    pub fn submit(&self, signal: Signal) -> Order {
        tracing::info!(?signal, "oms submit (mock)");
        Order {
            state: OrderState::Accepted,
            signal,
        }
    }
}
