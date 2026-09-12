//! 跑一下：4 个 worker 并发执行 8 个任务，然后优雅停机。
//!
//! ```bash
//! cargo run -p thread-pool-demo
//! cargo test -p thread-pool-demo
//! ```

use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::Arc;
use std::thread;
use std::time::{Duration, Instant};

use thread_pool_demo::ThreadPool;

fn main() {
    let workers = 4;
    let pool = ThreadPool::build(workers).expect("pool size must be > 0");
    println!("thread pool: {workers} workers, {:?} cores detected", thread::available_parallelism().map(|n| n.get()).ok());

    let done = Arc::new(AtomicUsize::new(0));
    let start = Instant::now();

    for i in 0..8 {
        let done = Arc::clone(&done);
        pool.execute(move || {
            // 模拟 100ms 的任务；8 个任务 / 4 worker ≈ 2 批 ≈ 200ms
            thread::sleep(Duration::from_millis(100));
            let n = done.fetch_add(1, Ordering::SeqCst) + 1;
            println!("  job {i} done (worker {:?}, total {n})", thread::current().id());
        })
        .expect("submit");
    }

    // Drop 在这里发生：先 drop(sender) 发停机信号，再 join 等所有 worker 收工。
    drop(pool);

    println!(
        "all {} jobs finished in {:?}; drop() returned => graceful shutdown ok",
        done.load(Ordering::SeqCst),
        start.elapsed()
    );
    assert_eq!(done.load(Ordering::SeqCst), 8);
}
