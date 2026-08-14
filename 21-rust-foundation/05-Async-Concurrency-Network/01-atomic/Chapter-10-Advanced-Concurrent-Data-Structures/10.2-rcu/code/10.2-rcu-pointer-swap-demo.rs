//! 10.2 RCU 思想：`AtomicPtr` 读当前版本，写者 CAS 替换。

use std::sync::atomic::{AtomicPtr, Ordering};

struct Config {
    value: i32,
}

pub fn demo() {
    let initial = Box::into_raw(Box::new(Config { value: 1 }));
    let global = AtomicPtr::new(initial);
    let old = global.load(Ordering::Acquire);
    let snapshot = unsafe { &*old };
    println!("read snapshot: {}", snapshot.value);
    let new_cfg = Box::into_raw(Box::new(Config { value: 2 }));
    let _ = global.compare_exchange(old, new_cfg, Ordering::AcqRel, Ordering::Acquire);
    println!(
        "after update: {}",
        unsafe { &*global.load(Ordering::Acquire) }.value
    );
    unsafe {
        drop(Box::from_raw(old));
    }
}
