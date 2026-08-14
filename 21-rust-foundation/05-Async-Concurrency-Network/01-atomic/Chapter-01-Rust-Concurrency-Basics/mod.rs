//! 第一章 — 书 §1.1～1.9 · 每节一目录 `1.Y-slug/`（索引 + 子笔记 + code/ demo）

#[path = "1.1-threads-in-rust/code/1.1-threads-in-rust-move-closure-demo.rs"]
pub mod move_closure;

#[path = "1.1-threads-in-rust/code/1.1-threads-in-rust-example-demo.rs"]
pub mod thread_example;

#[path = "1.1-threads-in-rust/code/1.1-threads-in-rust-usecall-demo.rs"]
pub mod usecall;

#[path = "1.1-threads-in-rust/code/1.1-threads-in-rust-join-demo.rs"]
pub mod usejoin;

#[path = "1.1-threads-in-rust/code/1.1-threads-in-rust-return-demo.rs"]
pub mod userecall;

#[path = "1.1-threads-in-rust/code/1.1-threads-in-rust-join-panic-demo.rs"]
pub mod usejoin_panic;

#[path = "1.1-threads-in-rust/code/1.1-threads-in-rust-static-vs-scope-demo.rs"]
pub mod use_static_vs_scope;

#[path = "1.1-threads-in-rust/code/1.1-threads-in-rust-fnonce-demo.rs"]
pub mod use_fnonce;

#[path = "1.2-scoped-threads/code/1.2-scoped-threads-demo.rs"]
pub mod usescope;

#[path = "1.3-shared-ownership/code/1.3-shared-ownership-static-demo.rs"]
pub mod usestatic;

#[path = "1.3-shared-ownership/code/1.3-shared-ownership-box-leak-demo.rs"]
pub mod useboxleak;

#[path = "1.3-shared-ownership/code/1.3-shared-ownership-arc-demo.rs"]
pub mod use_rc_arc;

#[path = "1.5-interior-mutability/code/1.5-interior-mutability-cell-refcell-demo.rs"]
pub mod use_cell_refcell;

#[path = "1.6-send-sync/code/1.6-send-sync-demo.rs"]
pub mod use_send_sync;

#[path = "1.7-mutex-rwlock/code/1.7-mutex-rwlock-mutex-demo.rs"]
pub mod use_mutex;

#[path = "1.7-mutex-rwlock/code/1.7-mutex-rwlock-guard-lifetime-demo.rs"]
pub mod use_mutex_guard_lifetime;

#[path = "1.7-mutex-rwlock/code/1.7-mutex-rwlock-thread-advanced-demo.rs"]
pub mod thread_advanced;

#[path = "1.8-parking-condvar/code/1.8-parking-condvar-demo.rs"]
pub mod use_condvar;
