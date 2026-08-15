//! 原子操作与 memory ordering。

use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};
use std::sync::Arc;
use std::thread;

pub fn relaxed_counter(threads: usize) -> usize {
    let counter = Arc::new(AtomicUsize::new(0));
    let handles: Vec<_> = (0..threads)
        .map(|_| {
            let c = counter.clone();
            thread::spawn(move || {
                for _ in 0..1_000 {
                    c.fetch_add(1, Ordering::Relaxed);
                }
            })
        })
        .collect();
    for h in handles {
        h.join().unwrap();
    }
    counter.load(Ordering::Relaxed)
}

/// Release（发布）+ Acquire（获取）建立跨线程 happens-before。
pub fn release_acquire_publish() -> usize {
    let data = Arc::new(AtomicUsize::new(0));
    let ready = Arc::new(AtomicBool::new(false));

    let data_p = data.clone();
    let ready_p = ready.clone();
    let producer = thread::spawn(move || {
        data_p.store(42, Ordering::Relaxed);
        ready_p.store(true, Ordering::Release);
    });

    while !ready.load(Ordering::Acquire) {
        std::hint::spin_loop();
    }
    let value = data.load(Ordering::Relaxed);
    producer.join().unwrap();
    value
}

pub fn seq_cst_counter(threads: usize) -> usize {
    let counter = Arc::new(AtomicUsize::new(0));
    let handles: Vec<_> = (0..threads)
        .map(|_| {
            let c = counter.clone();
            thread::spawn(move || {
                for _ in 0..1_000 {
                    c.fetch_add(1, Ordering::SeqCst);
                }
            })
        })
        .collect();
    for h in handles {
        h.join().unwrap();
    }
    counter.load(Ordering::SeqCst)
}
