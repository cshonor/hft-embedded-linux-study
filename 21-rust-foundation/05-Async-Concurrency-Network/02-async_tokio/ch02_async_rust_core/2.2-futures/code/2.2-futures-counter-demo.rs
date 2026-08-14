//! 2.2 Futures：手写计数器 Future，演示 `poll` → Pending / Ready。

use std::future::Future;
use std::pin::Pin;
use std::task::{Context, Poll};

struct CounterFuture {
    count: u32,
}

impl Future for CounterFuture {
    type Output = u32;

    fn poll(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        let this = self.get_mut();
        this.count += 1;
        println!("poll count: {}", this.count);
        if this.count < 5 {
            cx.waker().wake_by_ref();
            Poll::Pending
        } else {
            Poll::Ready(this.count)
        }
    }
}

#[tokio::main]
async fn main() {
    let n = CounterFuture { count: 0 }.await;
    println!("=== CounterFuture ready: {n} ===");
}
