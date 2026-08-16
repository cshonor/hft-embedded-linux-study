//! §7.3 — 教学 demo（Tokio）。
use tokio::time::{sleep, Duration};

#[tokio::main]
async fn main() {
    println!("{} — 见同章对应 .md 精读", "7.3-getting-unsafe-with-thread-data");
    sleep(Duration::from_millis(50)).await;
    println!("done");
}
