//! §3.5 — 教学 demo（Tokio）。
use tokio::time::{sleep, Duration};

#[tokio::main]
async fn main() {
    println!("{} — 见同章对应 .md 精读", "3.5-refactoring-spawn-task");
    sleep(Duration::from_millis(50)).await;
    println!("done");
}
