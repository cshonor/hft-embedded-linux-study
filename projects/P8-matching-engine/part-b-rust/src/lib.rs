//! p8-lob — P8 撮合引擎 Rust 版（Phase 1+2）
//!
//! 对照 C++ 版 [part-a-lob](../part-a-lob/)：同样的价格优先 + FIFO + 部分成交 + 撤单，
//! 加 Phase 2 的 Market/IOC/FOK。整个 crate **零 unsafe**。

pub mod book;
pub mod spsc;
pub mod types;

pub use book::Book;
pub use spsc::SpscRing;
pub use types::{Order, OrderType, Price, Qty, Side, Trade};
