//! Mutex poison：持锁 panic 后后续 lock 失败。

use std::sync::{Arc, Mutex};
use std::thread;

pub fn mutex_poisoned_after_panic() -> bool {
    let m = Arc::new(Mutex::new(0_i32));
    let m2 = m.clone();
    let handle = thread::spawn(move || {
        let _guard = m2.lock().unwrap();
        panic!("panic while holding mutex");
    });
    let _ = handle.join();
    let poisoned = m.lock().is_err();
    poisoned
}
