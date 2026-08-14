//! 10.1 计数信号量：`AtomicUsize` + `park`/`unpark` 简化版。

use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::Arc;
use std::thread;

pub struct Semaphore {
    count: AtomicUsize,
}

impl Semaphore {
    pub fn new(n: usize) -> Self {
        Self {
            count: AtomicUsize::new(n),
        }
    }

    pub fn wait(&self) {
        while self.count.fetch_sub(1, Ordering::AcqRel) == 0 {
            self.count.fetch_add(1, Ordering::Relaxed);
            thread::park();
        }
    }

    pub fn signal(&self) {
        if self.count.fetch_add(1, Ordering::Release) == 0 {
            thread::park_timeout(std::time::Duration::from_millis(1));
        }
    }
}

pub fn demo() {
    let sem = Arc::new(Semaphore::new(2));
    let mut handles = vec![];
    for i in 0..4 {
        let s = sem.clone();
        handles.push(thread::spawn(move || {
            s.wait();
            println!("worker {i} entered");
            thread::sleep(std::time::Duration::from_millis(20));
            s.signal();
        }));
    }
    for h in handles {
        h.join().unwrap();
    }
    println!("=== Semaphore demo done ===");
}
