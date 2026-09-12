//! 2.6 Sharing Data：`Arc<Mutex>` + 手写 `poll` 里用 `try_lock`（禁止阻塞运行时）。

use std::future::Future;
use std::pin::Pin;
use std::sync::{Arc, Mutex};
use std::task::{Context, Poll};

struct IncFuture {
    shared: Arc<Mutex<u32>>,
    done: bool,
}

impl Future for IncFuture {
    type Output = u32;

    fn poll(mut self: Pin<&mut Self>, _cx: &mut Context<'_>) -> Poll<Self::Output> {
        if self.done {
            return Poll::Ready(*self.shared.lock().unwrap());
        }
        // 把「加锁 + 自增」圈进独立作用域：guard 与 match 的临时值在块尾就释放，
        // 否则它们的不可变借用会一直活到函数尾，下面的 self.done = true 就会冲突（E0502）。
        let inc: Option<u32> = {
            match self.shared.try_lock() {
                Ok(mut g) => {
                    *g += 1;
                    Some(*g)
                }
                Err(_) => None,
            }
        };

        match inc {
            Some(value) => {
                self.done = true;
                Poll::Ready(value)
            }
            None => Poll::Pending,
        }
    }
}

#[tokio::main]
async fn main() {
    let n = Arc::new(Mutex::new(0u32));
    let a = IncFuture {
        shared: n.clone(),
        done: false,
    };
    let b = IncFuture {
        shared: n.clone(),
        done: false,
    };
    let (x, y) = tokio::join!(a, b);
    println!("=== try_lock inc results: {x}, {y} (order may vary) ===");
}
