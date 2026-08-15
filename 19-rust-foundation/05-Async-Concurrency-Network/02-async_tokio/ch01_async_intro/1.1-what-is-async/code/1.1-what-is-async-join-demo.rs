//! §1.1：串行 `await` vs `tokio::join!`（`sleep` 模拟 I/O 等待）。
use std::time::Instant;

use tokio::time::{sleep, Duration};

async fn task_a() {
    println!("Task A 开始");
    sleep(Duration::from_secs(1)).await;
    println!("Task A 完成");
}

async fn task_b() {
    println!("Task B 开始");
    sleep(Duration::from_secs(2)).await;
    println!("Task B 完成");
}

#[tokio::main]
async fn main() {
    println!("=== 同步执行（总耗时 ~3s）===");
    let start = Instant::now();
    task_a().await;
    task_b().await;
    println!("同步总耗时：{}ms", start.elapsed().as_millis());

    println!("\n=== 异步 join!（总耗时 ~2s）===");
    let start = Instant::now();
    tokio::join!(task_a(), task_b());
    println!("异步 join! 总耗时：{}ms", start.elapsed().as_millis());
}
