//! 2.7 High-Level Sharing：`tokio::sync::Mutex` + `async/await`（勿在 guard 上 `.await` std 锁）。

use std::sync::Arc;
use tokio::sync::Mutex;

async fn bump(label: &str, m: Arc<Mutex<u32>>) {
    let mut g = m.lock().await;
    *g += 1;
    println!("{label} -> {}", *g);
}

#[tokio::main]
async fn main() {
    let shared = Arc::new(Mutex::new(0u32));
    tokio::join!(
        bump("a", shared.clone()),
        bump("b", shared.clone()),
    );
    println!("=== final count: {} ===", *shared.lock().await);
}
