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
    let mut blocked_send = tokio::spawn(async move {
        tx2.send(3).await.unwrap();
    });

    // 用 &mut 借用：JoinHandle 是 Unpin，&mut F 仍是 Future，
    // 超时后句柄不会被 move 走，下面还能继续 await 它。
    // （原代码直接把 blocked_send 传进 timeout，之后又 await —— 编译不过，说明从未编译过。）
    let hung = timeout(Duration::from_millis(80), &mut blocked_send).await.is_err();
    assert!(hung, "channel full: third send should not finish yet");

    assert_eq!(rx.recv().await, Some(1));
    // spawn 的 async 块以 .unwrap() 结尾，返回 ()，不能再对 await 结果调 .unwrap()。
    blocked_send.await.expect("join");
    println!("§11.5 ok — after recv, blocked send completed");
}
