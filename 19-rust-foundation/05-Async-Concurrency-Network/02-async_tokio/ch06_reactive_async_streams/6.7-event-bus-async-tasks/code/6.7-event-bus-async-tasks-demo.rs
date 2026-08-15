//! §6.7：Tokio 发布/消费 + 有界通道背压示意。

use tokio::sync::mpsc;

#[tokio::main]
async fn main() {
    let (tx, mut rx) = mpsc::channel(4);
    let producer = tokio::spawn(async move {
        for i in 0..6 {
            if tx.send(i).await.is_err() {
                break;
            }
        }
    });
    let mut received = 0;
    while let Some(v) = rx.recv().await {
        received += 1;
        println!("got {v}");
    }
    let _ = producer.await;
    println!("§6.7 ok: received {received} (channel caps fan-out backpressure)");
}
