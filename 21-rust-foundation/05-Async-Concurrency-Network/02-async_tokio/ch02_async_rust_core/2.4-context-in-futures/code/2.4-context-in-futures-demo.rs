//! §2.4 — 教学 demo（Tokio）。
use tokio::time::{sleep, Duration};

#[tokio::main]
async fn main() {
    println!("{} — 见同章对应 .md 精读", "2.4-context-in-futures");
    sleep(Duration::from_millis(50)).await;
    println!("done");
}
