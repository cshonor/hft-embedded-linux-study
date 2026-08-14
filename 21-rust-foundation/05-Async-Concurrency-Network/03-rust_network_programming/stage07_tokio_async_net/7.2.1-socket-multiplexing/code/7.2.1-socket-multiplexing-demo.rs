//! §7.2.1：多 task 并发，模拟 RequestId 乱序完成。
use tokio::time::{sleep, Duration};

async fn handle(id: u32, delay_ms: u64) {
    sleep(Duration::from_millis(delay_ms)).await;
    println!("response id={id} (delay {delay_ms}ms)");
}

#[tokio::main]
async fn main() {
    let mut handles = vec![];
    for (id, delay) in [(1, 300), (2, 100), (3, 200)] {
        handles.push(tokio::spawn(handle(id, delay)));
    }
    for h in handles {
        let _ = h.await;
    }
}
