//! 5.1 基于 Mutex 的简单通道（教学向）：`VecDeque` + `Mutex` + `Condvar`。

use std::collections::VecDeque;
use std::sync::{Arc, Condvar, Mutex};
use std::thread;
use std::time::Duration;

pub struct MutexChannel<T> {
    inner: Arc<Inner<T>>,
}

struct Inner<T> {
    queue: Mutex<VecDeque<T>>,
    ready: Condvar,
}

impl<T> Clone for MutexChannel<T> {
    fn clone(&self) -> Self {
        Self {
            inner: Arc::clone(&self.inner),
        }
    }
}

impl<T> MutexChannel<T> {
    pub fn new() -> Self {
        Self {
            inner: Arc::new(Inner {
                queue: Mutex::new(VecDeque::new()),
                ready: Condvar::new(),
            }),
        }
    }

    pub fn send(&self, value: T) {
        let mut q = self.inner.queue.lock().unwrap();
        q.push_back(value);
        self.inner.ready.notify_one();
    }

    pub fn recv(&self) -> T {
        let mut q = self.inner.queue.lock().unwrap();
        while q.is_empty() {
            q = self.inner.ready.wait(q).unwrap();
        }
        q.pop_front().unwrap()
    }
}

pub fn demo() {
    let ch = MutexChannel::new();
    let producer = ch.clone();
    let handle = thread::spawn(move || {
        thread::sleep(Duration::from_millis(30));
        producer.send(100);
    });
    let v = ch.recv();
    handle.join().unwrap();
    println!("=== Mutex-based channel: received {v} (expect 100) ===");
}
