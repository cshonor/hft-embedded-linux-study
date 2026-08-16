//! §3.8 — 教学 demo（Tokio）。
use tokio::time::{sleep, Duration};

#[tokio::main]
async fn main() {
    println!("{} — 见同章对应 .md 精读", "3.8-running-background-processes");
    sleep(Duration::from_millis(50)).await;
    println!("done");
}
