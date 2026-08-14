//! 第二章 — 书 §2.1～2.4 · 每节 `2.Y-slug/code/` demo

#[path = "2.1-atomic-load-store/code/2.1-atomic-load-store-demo.rs"]
pub mod use_atomic;

#[path = "2.1-atomic-load-store/code/2.1-atomic-load-store-lazy-init-demo.rs"]
pub mod lazy_init;

#[path = "2.2-fetch-and-modify/code/2.2-fetch-and-modify-demo.rs"]
pub mod use_atomic_operations;

#[path = "2.3-compare-and-exchange/code/2.3-compare-and-exchange-id-allocator-demo.rs"]
pub mod id_allocator;

#[path = "2.4-summary/code/2.4-summary-demo.rs"]
pub mod quick_demo;
