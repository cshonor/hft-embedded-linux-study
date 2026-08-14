//! 2.1 Understanding Tasks：`tokio::join!` 并发多个不阻塞 CPU 的异步操作。

use tokio::time::{sleep, Duration};

async fn work(id: u32, ms: u64) -> u32 {
    sleep(Duration::from_millis(ms)).await;
    println!("task {id} done");
    id
}

#[tokio::main]
async fn main() {
    let (a, b, c) = tokio::join!(work(1, 50), work(2, 30), work(3, 10));
    println!("=== join! results: {a}, {b}, {c} ===");
}
