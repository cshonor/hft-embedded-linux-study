//! 第 5 章：单次发送通道（One-Shot Channel）教学实现。
//!
//! 综合 **CAS / Release-Acquire / `MaybeUninit` / `park`–`unpark`**；
//! 与 `std::sync::mpsc` 或 crate `oneshot` 相比，仅用于理解原理，非生产替代。

use std::cell::UnsafeCell;
use std::mem::MaybeUninit;
use std::sync::atomic::{AtomicBool, Ordering};
use std::thread::{self, Thread};

pub struct Channel<T> {
    message: UnsafeCell<MaybeUninit<T>>,
    ready: AtomicBool,
    /// 接收方在 `park` 前登记，供发送方 `unpark`。
    receiver_thread: UnsafeCell<Option<Thread>>,
}

unsafe impl<T: Send> Sync for Channel<T> {}

impl<T> Channel<T> {
    pub const fn new() -> Self {
        Self {
            message: UnsafeCell::new(MaybeUninit::uninit()),
            ready: AtomicBool::new(false),
            receiver_thread: UnsafeCell::new(None),
        }
    }

    /// 重置内部状态并拆成 `Sender` / `Receiver`（各仅能使用一次）。
    pub fn split<'a>(&'a mut self) -> (Sender<'a, T>, Receiver<'a, T>) {
        *self = Self::new();
        (Sender { channel: self }, Receiver { channel: self })
    }
}

pub struct Sender<'a, T> {
    channel: &'a Channel<T>,
}

pub struct Receiver<'a, T> {
    channel: &'a Channel<T>,
}

impl<T> Sender<'_, T> {
    /// 消耗 `self`，保证至多发送一次（类型系统）。
    pub fn send(self, message: T) {
        unsafe {
            (*self.channel.message.get()).write(message);
        }
        self.channel.ready.store(true, Ordering::Release);
        // SAFETY: 仅接收方写入 `receiver_thread`。
        if let Some(t) = unsafe { (*self.channel.receiver_thread.get()).take() } {
            t.unpark();
        }
    }
}

impl<T> Receiver<'_, T> {
    /// 消耗 `self`；未就绪时 `park` 让出 CPU（与自旋锁对比）。
    pub fn receive(self) -> T {
        unsafe {
            *self.channel.receiver_thread.get() = Some(thread::current());
        }
        while !self.channel.ready.load(Ordering::Acquire) {
            thread::park();
        }
        self.channel.ready.store(false, Ordering::Relaxed);
        unsafe { (*self.channel.message.get()).assume_init_read() }
    }
}

pub fn demo() {
    // 教学 demo：`split` 借的是栈上 `channel`，`Sender` 进 `spawn` 需 `'static`。
    // 书中常在同一线程或 scope 内完成；此处用 `Box::leak` 仅演示跨线程 send/receive。
    let channel: &'static mut Channel<u32> = Box::leak(Box::new(Channel::new()));
    let (tx, rx) = channel.split();

    let handle = thread::spawn(move || {
        thread::sleep(std::time::Duration::from_millis(50));
        tx.send(42);
    });

    let value = rx.receive();
    handle.join().unwrap();
    println!("=== One-Shot Channel demo: 收到 {value}（期望 42）===");
}
