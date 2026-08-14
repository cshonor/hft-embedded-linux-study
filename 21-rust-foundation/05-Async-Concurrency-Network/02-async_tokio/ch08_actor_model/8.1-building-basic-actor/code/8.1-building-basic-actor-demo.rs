//! §8.1：Actor 邮箱 + `oneshot` 请求—响应。
use tokio::sync::{mpsc, oneshot};

struct RespMessage {
    value: i64,
    responder: oneshot::Sender<i64>,
}

async fn resp_actor(mut receiver: mpsc::Receiver<RespMessage>) {
    while let Some(msg) = receiver.recv().await {
        let result = msg.value + 1;
        let _ = msg.responder.send(result);
    }
}

#[tokio::main]
async fn main() {
    let (tx, rx) = mpsc::channel::<RespMessage>(8);
    tokio::spawn(resp_actor(rx));

    let (resp_tx, resp_rx) = oneshot::channel();
    tx.send(RespMessage {
        value: 41,
        responder: resp_tx,
    })
    .await
    .expect("send to actor");

    let answer = resp_rx.await.expect("response");
    println!("actor 返回: {answer}");
}
