//! 2.5 Waking Remotely：在通道侧保存 `Waker`，收到消息后再唤醒 Future。

use std::future::Future;
use std::pin::Pin;
use std::sync::{Arc, Mutex};
use std::task::{Context, Poll, Waker};
use tokio::sync::mpsc;

struct RemoteWake {
    rx: mpsc::Receiver<String>,
    waker_slot: Arc<Mutex<Option<Waker>>>,
}

impl Future for RemoteWake {
    type Output = String;

    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        if let Ok(msg) = self.rx.try_recv() {
            return Poll::Ready(msg);
        }
        *self.waker_slot.lock().unwrap() = Some(cx.waker().clone());
        Poll::Pending
    }
}

#[tokio::main]
async fn main() {
    let (tx, rx) = mpsc::channel(1);
    let slot = Arc::new(Mutex::new(None::<Waker>));
    let fut = RemoteWake {
        rx,
        waker_slot: slot.clone(),
    };
    let handle = tokio::spawn(async move {
        tokio::time::sleep(std::time::Duration::from_millis(40)).await;
        tx.send("hello from channel".into()).await.unwrap();
        if let Some(w) = slot.lock().unwrap().take() {
            w.wake();
        }
    });
    let msg = fut.await;
    handle.await.unwrap();
    println!("=== Remote wake: {msg} ===");
}
