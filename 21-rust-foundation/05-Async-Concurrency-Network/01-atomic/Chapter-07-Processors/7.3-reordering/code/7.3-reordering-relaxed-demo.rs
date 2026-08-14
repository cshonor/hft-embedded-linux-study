//! 7.3 重排感知：Relaxed 下两原子变量可能以意外顺序被观察到（教学打印）。

use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::Arc;
use std::thread;

pub fn demo() {
    let x = Arc::new(AtomicUsize::new(0));
    let y = Arc::new(AtomicUsize::new(0));
    let x1 = x.clone();
    let y1 = y.clone();
    let t1 = thread::spawn(move || {
        x1.store(1, Ordering::Relaxed);
        let _ = y1.load(Ordering::Relaxed);
    });
    let x2 = x.clone();
    let y2 = y.clone();
    let t2 = thread::spawn(move || {
        y2.store(1, Ordering::Relaxed);
        let _ = x2.load(Ordering::Relaxed);
    });
    t1.join().unwrap();
    t2.join().unwrap();
    println!(
        "=== Reordering demo: x={}, y={} (both 1; order of stores may vary on weak ISAs) ===",
        x.load(Ordering::Relaxed),
        y.load(Ordering::Relaxed)
    );
}
