//! `Send` / `Sync`：自动推导与重要例外。

use std::cell::UnsafeCell;
use std::rc::Rc;
use std::sync::{Arc, Mutex};

fn assert_send<T: Send>() {}
fn assert_sync<T: Sync>() {}

pub fn compile_time_bounds() {
    assert_send::<i32>();
    assert_sync::<i32>();

    assert_send::<Arc<Mutex<Vec<u8>>>>();
    assert_sync::<Arc<Mutex<Vec<u8>>>>();

    // 非 Send / Sync（取消注释即编译失败）：
    // assert_send::<Rc<i32>>();
    // assert_sync::<Rc<i32>>();
    // assert_send::<*const i32>();
    // assert_sync::<UnsafeCell<i32>>();
}

/// `T: Sync` 当且仅当 `&T: Send`（语言定义）。
pub fn sync_means_shared_ref_is_send() {
    fn needs_send<T: Send>() {}
    fn check_sync<T: Sync>() {
        needs_send::<&T>();
    }
    check_sync::<i32>();
}

pub fn non_thread_safe_types_note() -> &'static str {
    "raw ptr, UnsafeCell, Rc — see 00-overview.md §2"
}

// 使 `UnsafeCell` / `Rc` 在文档中可引用而不 warn
#[allow(dead_code)]
fn _mention_exceptions() {
    let _: UnsafeCell<i32> = UnsafeCell::new(0);
    let _: Rc<i32> = Rc::new(0);
}
