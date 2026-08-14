//! §11.7：手动两次 poll（Pending → Ready），对照 tokio-test 的 assert_pending。

use std::future::Future;
use std::pin::Pin;
use std::task::{Context, Poll, RawWaker, RawWakerVTable, Waker};

struct CountToTwo {
    n: u32,
}

impl Future for CountToTwo {
    type Output = u32;
    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        self.n += 1;
        if self.n < 2 {
            cx.waker().wake_by_ref();
            Poll::Pending
        } else {
            Poll::Ready(self.n)
        }
    }
}

fn noop_waker() -> Waker {
    static VT: RawWakerVTable = RawWakerVTable::new(
        |_| RawWaker::new(std::ptr::null(), &VT),
        |_| {},
        |_| {},
        |_| {},
    );
    unsafe { Waker::from_raw(RawWaker::new(std::ptr::null(), &VT)) }
}

fn main() {
    let waker = noop_waker();
    let mut cx = Context::from_waker(&waker);
    let mut fut = CountToTwo { n: 0 };
    assert!(matches!(
        Pin::new(&mut fut).poll(&mut cx),
        Poll::Pending
    ));
    assert!(matches!(Pin::new(&mut fut).poll(&mut cx), Poll::Ready(2)));
    println!("§11.7 ok — 生产环境用 tokio_test::assert_pending!");
}
