//! 标准库原子类型：`load` / `store` / `fetch_add` / `compare_exchange`。

use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};
use std::sync::Arc;
use std::thread;

pub fn demo() {
    println!("=== atomic（std::sync::atomic）===");

    let done = Arc::new(AtomicBool::new(false));
    let count = Arc::new(AtomicUsize::new(0));

    let d = done.clone();
    let c = count.clone();
    thread::spawn(move || {
        for _ in 0..5 {
            c.fetch_add(1, Ordering::Relaxed);
        }
        d.store(true, Ordering::Release);
    });

    while !done.load(Ordering::Acquire) {
        thread::yield_now();
    }
    println!("fetch_add 累计: {}", count.load(Ordering::Relaxed));

    let x = AtomicUsize::new(0);
    match x.compare_exchange(0, 1, Ordering::AcqRel, Ordering::Relaxed) {
        Ok(old) => println!("CAS Ok，旧值: {}", old),
        Err(v) => println!("CAS Err，当前: {}", v),
    }
    println!("CAS 后 load: {}", x.load(Ordering::Relaxed));
}
