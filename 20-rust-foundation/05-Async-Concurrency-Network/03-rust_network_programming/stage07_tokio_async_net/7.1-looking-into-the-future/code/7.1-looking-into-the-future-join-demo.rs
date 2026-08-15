//! §7.1：`tokio::join!` 并发 sleep（理解 Future 并发）。
use std::time::Instant;

use tokio::time::{sleep, Duration};

async fn task(name: &str, secs: u64) {
    println!("{name} start");
    sleep(Duration::from_secs(secs)).await;
    println!("{name} done");
}

#[tokio::main]
async fn main() {
    let start = Instant::now();
    tokio::join!(task("A", 1), task("B", 2));
    println!("join elapsed: {}ms", start.elapsed().as_millis());
}
