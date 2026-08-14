//! 7.2 伪共享：相邻 `AtomicUsize` 在同一 cache line 上争用。

use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::Arc;
use std::thread;
use std::time::Instant;

const N: usize = 4;
const ITERS: u32 = 2_000_000;

pub fn demo() {
    let counters: Arc<Vec<AtomicUsize>> =
        Arc::new((0..N).map(|_| AtomicUsize::new(0)).collect());
    let start = Instant::now();
    let handles: Vec<_> = (0..N)
        .map(|i| {
            let c = counters.clone();
            thread::spawn(move || {
                for _ in 0..ITERS {
                    c[i].fetch_add(1, Ordering::Relaxed);
                }
            })
        })
        .collect();
    for h in handles {
        h.join().unwrap();
    }
    let elapsed = start.elapsed();
    let total: usize = counters.iter().map(|c| c.load(Ordering::Relaxed)).sum();
    println!(
        "=== False sharing demo: {N} threads × {ITERS} inc, total={total}, {:?} ===",
        elapsed
    );
}
