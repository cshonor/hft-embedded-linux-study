//! 和 P10 `types.hpp` 同一套语义。
//!
//! 新手先记住三件事：
//! 1. 价格不用 `f64`。0.1+0.2 在浮点里不等于 0.3，撮合会对不齐。
//!    `Price` 是整数 tick：1 tick = 0.01，所以 100.00 元写成 10000。
//! 2. 买卖是两个方向：Buy 想买（出价），Sell 想卖（要价）。
//! 3. 市场上有两类人：`OWNER_MARKET` 是「别人」（回放），`OWNER_US` 是「我们」（策略）。

pub type Price = i64;
pub type Qty = i64;

/// 簿上没有买/卖时的哨兵。
pub const INVALID_PRICE: Price = -1;
/// 市价买单：当成「无限高」去吃卖盘。
pub const MAX_PRICE: Price = 1_000_000_000;

pub const OWNER_MARKET: u32 = 0;
pub const OWNER_US: u32 = 1;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Side {
    Buy,
    Sell,
}

/// Limit  限价：能成交就成交，剩下来挂在簿上排队（提供流动性）
/// Market 市价：不指定价格，能吃多少吃多少，不挂单
/// Ioc    Immediate-or-Cancel：限价立刻成交，剩余扔掉，不挂
/// Fok    Fill-or-Kill：要么一次全成，要么整单取消
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum OrderType {
    Limit,
    Market,
    Ioc,
    Fok,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Order {
    pub id: u64,
    pub owner: u32,
    pub side: Side,
    pub ty: OrderType,
    pub price: Price,
    pub qty: Qty,
    pub ts: u64,
}

impl Order {
    pub fn new(id: u64, owner: u32, side: Side, ty: OrderType, price: Price, qty: Qty) -> Self {
        Self {
            id,
            owner,
            side,
            ty,
            price,
            qty,
            ts: 0,
        }
    }
}

impl Default for Order {
    fn default() -> Self {
        Self::new(0, OWNER_MARKET, Side::Buy, OrderType::Limit, 0, 0)
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum EventKind {
    Order,
    Cancel,
    Shutdown,
}

/// 回放 / 行情线程 → 引擎的消息。
/// `run_strategy=true` 表示「这一拍市场动作结束了，该让我们报价了」。
#[derive(Clone, Debug)]
pub struct Event {
    pub kind: EventKind,
    pub order: Order,
    pub cancel_id: u64,
    pub run_strategy: bool,
}

impl Event {
    pub fn order(order: Order, run_strategy: bool) -> Self {
        Self {
            kind: EventKind::Order,
            order,
            cancel_id: 0,
            run_strategy,
        }
    }

    pub fn cancel(id: u64) -> Self {
        Self {
            kind: EventKind::Cancel,
            order: Order::default(),
            cancel_id: id,
            run_strategy: false,
        }
    }

    pub fn shutdown() -> Self {
        Self {
            kind: EventKind::Shutdown,
            order: Order::default(),
            cancel_id: 0,
            run_strategy: false,
        }
    }
}

/// 一笔成交。taker = 主动来吃的单（刚 submit 的）；
/// maker = 已经挂在簿上被吃掉的单。
/// 成交价用 maker 的挂单价（价格改善归 taker）。
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Trade {
    pub taker_id: u64,
    pub maker_id: u64,
    pub taker_owner: u32,
    pub maker_owner: u32,
    pub taker_side: Side,
    pub price: Price,
    pub qty: Qty,
}
