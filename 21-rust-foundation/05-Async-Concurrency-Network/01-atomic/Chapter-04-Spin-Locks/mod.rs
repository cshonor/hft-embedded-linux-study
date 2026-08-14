//! 第 4 章 — 自旋锁（§4.1～4.3 同一实现递进）

#[path = "4.1-minimal-implementation/code/4.1-minimal-implementation-demo.rs"]
pub mod spin_lock;

pub fn demo() {
    spin_lock::demo();
}
