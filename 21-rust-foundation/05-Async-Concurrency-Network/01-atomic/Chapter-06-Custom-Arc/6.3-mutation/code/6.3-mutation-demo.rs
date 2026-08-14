//! 6.3 可变数据：`Arc::get_mut` 仅单所有者；多所有者用 `Arc<Mutex<T>>`。

use std::sync::{Arc, Mutex};

pub fn demo() {
    let mut solo = Arc::new(1);
    *Arc::get_mut(&mut solo).unwrap() += 1;
    println!("get_mut solo: {solo}");

    let shared = Arc::new(Mutex::new(0));
    *shared.lock().unwrap() = 7;
    println!("Arc<Mutex<T>> shared: {}", *shared.lock().unwrap());
}
