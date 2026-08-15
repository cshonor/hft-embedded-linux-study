//! 第 6 章 — 自定义 Arc

#[path = "6.1-basic-reference-counting/code/6.1-basic-reference-counting-demo.rs"]
pub mod custom_arc;

#[path = "6.3-mutation/code/6.3-mutation-demo.rs"]
pub mod arc_mutex;

#[path = "6.2-testing-it/code/6.2-testing-it-demo.rs"]
pub mod testing;

#[path = "6.4-weak-pointers/code/6.4-weak-pointers-demo.rs"]
pub mod weak_arc;

#[path = "6.6-optimizing/code/6.6-optimizing-align-demo.rs"]
pub mod optimizing;

pub fn demo() {
    custom_arc::demo();
    testing::demo();
    arc_mutex::demo();
    weak_arc::demo();
    optimizing::demo();
}
