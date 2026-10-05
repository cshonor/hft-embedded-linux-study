//! 基础类型（P8 Phase 1 骨架的 Rust 版，与 part-a `types` 语义对齐）
//!
//! 价格用整数（×10000 避免浮点），数量同理。
//! 所有类型都是小的 `Copy` 结构——热路径禁 clone（18-rust-quant ch02 纪律）。

/// 价格 ×10000（如 103.00 → 1030000）
pub type Price = i64;
/// 数量
pub type Qty = i64;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Side {
    Buy,
    Sell,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum OrderType {
    /// 限价单：吃对手簿到价格边界，剩余挂簿
    Limit,
    /// 市价单：不检查价格，吃完或吃净对手簿为止
    Market,
    /// Immediate-Or-Cancel：限价撮合，剩余立即撤销（不挂簿）
    Ioc,
    /// Fill-Or-Kill：不能全部成交就全部撤销（先预扫描，不部分成交）
    Fok,
}

#[derive(Debug, Clone, Copy)]
pub struct Order {
    pub id: u64,
    pub side: Side,
    pub otype: OrderType,
    pub price: Price,
    pub qty: Qty,
}

impl Order {
    pub fn limit(id: u64, side: Side, price: Price, qty: Qty) -> Self {
        Self { id, side, otype: OrderType::Limit, price, qty }
    }
    pub fn market(id: u64, side: Side, qty: Qty) -> Self {
        // 市价单的价格字段不参与撮合（买单视为 +∞，卖单视为 -∞）
        Self { id, side, otype: OrderType::Market, price: 0, qty }
    }
    pub fn ioc(id: u64, side: Side, price: Price, qty: Qty) -> Self {
        Self { id, side, otype: OrderType::Ioc, price, qty }
    }
    pub fn fok(id: u64, side: Side, price: Price, qty: Qty) -> Self {
        Self { id, side, otype: OrderType::Fok, price, qty }
    }
}

/// 一笔成交（taker 吃 maker）
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Trade {
    pub taker_id: u64,
    pub maker_id: u64,
    /// 成交价 = maker 挂单价（价格优先，taker 吃到的是对手簿价格）
    pub price: Price,
    pub qty: Qty,
}
