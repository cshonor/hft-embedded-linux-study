//! §7.1.1：`mpsc` 模拟 Stream 消费。
use tokio::sync::mpsc;

#[tokio::main]
async fn main() {
    let (tx, mut rx) = mpsc::channel(4);
    for i in 0..3 {
        tx.send(format!("frame-{i}")).await.unwrap();
    }
    drop(tx);
    while let Some(item) = rx.recv().await {
        println!("sink got: {item}");
    }
}
