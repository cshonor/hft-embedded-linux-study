use crate::strategy::Signal;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum OrderState {
    /// 已生成但尚未被 OMS 受理
    New,
    /// 已被 OMS 受理（mock 环境视为立即成交）
    Accepted,
}

#[derive(Debug)]
pub struct Order {
    pub id: u64,
    pub state: OrderState,
    pub signal: Signal,
}

pub struct Oms {
    next_id: u64,
}

impl Oms {
    pub fn new() -> Self {
        Self { next_id: 1 }
    }

    /// 受理流程：`New` → 校验 → `Accepted`。
    /// 保留 `New` 这一态，是因为真实 OMS 的撤单/拒绝都发生在它之后。
    pub fn submit(&mut self, signal: Signal) -> Order {
        let id = self.next_id;
        self.next_id += 1;

        let mut order = Order {
            id,
            state: OrderState::New,
            signal,
        };
        tracing::info!(order_id = id, ?order.signal, "oms received");

        order.state = OrderState::Accepted;
        tracing::info!(order_id = id, ?order.state, "oms accepted");

        order
    }
}

impl Default for Oms {
    fn default() -> Self {
        Self::new()
    }
}
