//! §9.4 — 教学 demo（Tokio）。
use tokio::time::{sleep, Duration};

#[tokio::main]
async fn main() {
    println!("{} — 见同章对应 .md 精读", "9.4-state-machine-pattern");
    sleep(Duration::from_millis(50)).await;
    println!("done");
}
