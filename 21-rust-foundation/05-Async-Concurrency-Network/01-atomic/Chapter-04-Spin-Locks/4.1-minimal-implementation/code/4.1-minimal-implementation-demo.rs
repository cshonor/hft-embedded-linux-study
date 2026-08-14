//! 第 4 章：最小自旋锁 —— `Acquire` / `Release` 的经典用法（与书中结构一致）。
//!
//! - **加锁**：`swap(true, Acquire)` 成功表示抢到锁；`Acquire` 保证之后访问 `value` 不会重排到加锁之前。
//! - **解锁**：`store(false, Release)`；`Release` 保证对 `value` 的修改不会重排到解锁之后。
//! - 下一线程加锁的 `Acquire` 与本次 `Release` 形成 **happens-before**，保护 `UnsafeCell` 内数据。

use std::cell::UnsafeCell;
use std::ops::{Deref, DerefMut};
use std::sync::atomic::{AtomicBool, Ordering};

pub struct SpinLock<T> {
    locked: AtomicBool,
    value: UnsafeCell<T>,
}

// 锁可被多线程共享；T 必须 Send（跨线程移动值）。
unsafe impl<T: Send> Sync for SpinLock<T> {}
unsafe impl<T: Send> Send for SpinLock<T> {}

pub struct Guard<'a, T> {
    lock: &'a SpinLock<T>,
}

impl<T> SpinLock<T> {
    pub const fn new(value: T) -> Self {
        Self {
            locked: AtomicBool::new(false),
            value: UnsafeCell::new(value),
        }
    }

    /// 书中常用：`swap(true, Acquire)` 抢锁。
    pub fn lock(&self) -> Guard<'_, T> {
        while self.locked.swap(true, Ordering::Acquire) {
            std::hint::spin_loop();
        }
        Guard { lock: self }
    }

    /// CAS 抢锁：`compare_exchange_weak(false, true, …)`，与 `lock()` 对照阅读。
    pub fn lock_cas(&self) -> Guard<'_, T> {
        while self
            .locked
            .compare_exchange_weak(false, true, Ordering::Acquire, Ordering::Relaxed)
            .is_err()
        {
            std::hint::spin_loop();
        }
        Guard { lock: self }
    }

    /// 仅由 `Guard::drop` 调用；对外用 RAII，勿重复 unlock。
    unsafe fn unlock(&self) {
        self.locked.store(false, Ordering::Release);
    }
}

impl<T> Drop for Guard<'_, T> {
    fn drop(&mut self) {
        // SAFETY: 当前线程持有锁，且不再持有 Guard。
        unsafe { self.lock.unlock() };
    }
}

impl<T> Deref for Guard<'_, T> {
    type Target = T;
    fn deref(&self) -> &T {
        // SAFETY: 已用 Acquire 成功加锁，无其他线程可访问 value。
        unsafe { &*self.lock.value.get() }
    }
}

impl<T> DerefMut for Guard<'_, T> {
    fn deref_mut(&mut self) -> &mut T {
        unsafe { &mut *self.lock.value.get() }
    }
}

/// 两线程对同一 `SpinLock` 累加，演示 Acquire/Release 保护共享数据。
pub fn demo() {
    use std::sync::Arc;
    use std::thread;

    let lock = Arc::new(SpinLock::new(0u32));
    let mut handles = vec![];

    for _ in 0..4 {
        let lock = Arc::clone(&lock);
        handles.push(thread::spawn(move || {
            for _ in 0..1000 {
                let mut guard = lock.lock();
                *guard += 1;
            }
        }));
    }

    for h in handles {
        h.join().unwrap();
    }

    let guard = lock.lock();
    println!(
        "=== SpinLock demo: 4 线程 × 1000 次 += 1 → 期望 4000，实际 {} ===",
        *guard
    );
}
