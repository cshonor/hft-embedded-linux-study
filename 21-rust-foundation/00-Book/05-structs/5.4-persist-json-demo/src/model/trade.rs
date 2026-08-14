use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct Trade {
    pub order_id: u64,
    pub code: String,
    pub price: f64,
    pub volume: u64,
    pub timestamp: u64,
}
