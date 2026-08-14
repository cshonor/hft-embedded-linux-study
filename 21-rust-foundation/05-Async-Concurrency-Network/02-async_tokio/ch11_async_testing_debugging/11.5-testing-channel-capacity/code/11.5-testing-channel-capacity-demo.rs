//! §11.5：有界通道满时 send 阻塞 + timeout 断言。

use std::time::Duration;
use tokio::sync::mpsc;
use tokio::time::timeout;

#[tokio::main]
async fn main() {
    let (tx, mut rx) = mpsc::channel(2);
    tx.send(1).await.unwrap();
    tx.send(2).await.unwrap();

    let tx2 = tx.clone();
    let blocked_send = tokio::spawn(async move {
        tx2.send(3).await.unwrap();
    });

    let hung = timeout(Duration::from_millis(80), blocked_send).await.is_err();
    assert!(hung, "channel full: third send should not finish yet");

    assert_eq!(rx.recv().await, Some(1));
    blocked_send.await.expect("join").unwrap();
    println!("§11.5 ok — after recv, blocked send completed");
}
