//! §10.2：最小 `RawWaker` + `block_on`（仅 `std`）。
//!
//! `Waker` 为 no-op：`Pending` 时由外层循环 `yield_now` 再次 `poll`——仅用于理解，非高效调度。
use std::future::Future;
use std::pin::Pin;
use std::task::{Context, Poll, RawWaker, RawWakerVTable, Waker};
use std::thread;

fn noop_clone(p: *const ()) -> RawWaker {
    RawWaker::new(p, &NOOP_VTABLE)
}
fn noop_wake(_: *const ()) {}
fn noop_wake_by_ref(_: *const ()) {}
fn noop_drop(_: *const ()) {}

static NOOP_VTABLE: RawWakerVTable =
    RawWakerVTable::new(noop_clone, noop_wake, noop_wake_by_ref, noop_drop);

fn noop_waker() -> Waker {
    // SAFETY: `NOOP_VTABLE` 满足 `RawWaker` 契约（空指针 data）。
    unsafe { Waker::from_raw(RawWaker::new(std::ptr::null(), &NOOP_VTABLE)) }
}

/// 若干次 `Pending` 后返回 `Ready`。
struct Countdown(u32);

impl Future for Countdown {
    type Output = &'static str;

    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        if self.0 == 0 {
            Poll::Ready("done")
        } else {
            self.0 -= 1;
            cx.waker().wake_by_ref();
            Poll::Pending
        }
    }
}

fn block_on<F: Future>(f: F) -> F::Output {
    let mut f = Box::pin(f);
    let waker = noop_waker();
    let mut cx = Context::from_waker(&waker);
    loop {
        match f.as_mut().poll(&mut cx) {
            Poll::Ready(v) => return v,
            Poll::Pending => thread::yield_now(),
        }
    }
}

fn main() {
    let msg = block_on(Countdown(3));
    println!("{msg}");
}
