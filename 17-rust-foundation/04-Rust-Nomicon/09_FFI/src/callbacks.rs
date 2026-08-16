//! 回调：全局 fn 指针与 `*mut` 携带 Rust 状态。

use std::cell::Cell;

pub type CCallback = extern "C" fn(i32) -> i32;

struct Counter(Cell<i32>);

/// 模拟 C 库：注册回调并调用一次。
pub fn simulate_c_invoke(cb: CCallback, arg: i32) -> i32 {
    cb(arg)
}

extern "C" fn double_cb(x: i32) -> i32 {
    x * 2
}

extern "C" fn increment_via_ptr(state: *mut Counter, x: i32) -> i32 {
    assert!(!state.is_null());
    unsafe {
        let c = &*state;
        c.0.set(c.0.get() + x);
        c.0.get()
    }
}

pub fn demo_global_callback() -> i32 {
    simulate_c_invoke(double_cb, 21)
}

pub fn demo_stateful_callback() -> i32 {
    let mut counter = Counter(Cell::new(0));
    let cb: extern "C" fn(*mut Counter, i32) -> i32 = increment_via_ptr;
    let v1 = cb(&mut counter as *mut Counter, 10);
    let v2 = cb(&mut counter as *mut Counter, 5);
    v1 + v2
}

// 异线程回调：须 Mutex/通道 + 销毁前注销 — 见 00-overview.md §3。
