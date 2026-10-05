//! SPSC 无锁环形队列（P8 Phase 3）
//!
//! Single Producer Single Consumer：一个线程只写，一个线程只读。
//! 为什么不用互斥锁？锁会让线程去睡觉（上下文切换 µs 级），HFT 热路径受不了。
//! 这里用「固定数组 + 两个原子下标」：写之前看是不是满了，读之前看是不是空了。
//!
//! 内存序（对应 14-hft ch07 / C++ Concurrency ch05）：
//!   先把数据写进 slots[...]，再 release 地 head++；
//!   读者 acquire 地读 head，才能保证看见刚才那份数据。反了顺序就会读到半成品。
//!
//! **unsafe 边界（18-rust-quant ch02 纪律）：本 crate 仅有的 unsafe 就在
//! `try_push` / `try_pop` / `Drop` 里**——别处（撮合、策略、风控）全是安全 Rust。

use std::cell::UnsafeCell;
use std::mem::MaybeUninit;
use std::sync::atomic::{AtomicUsize, Ordering};

/// head / tail 各占一条 cache line：挤在同一 64B 里的话，
/// 写 head 会把对方的 tail 缓存行挤出去（伪共享，见 06.6.5/ch13 false_sharing_demo）
#[repr(C, align(64))]
struct Padded(AtomicUsize);

impl Padded {
    const fn new(v: usize) -> Self {
        Self(AtomicUsize::new(v))
    }
}

/// 固定容量 SPSC ring。`N` 必须是 2 的幂（下标用 `& (N-1)` 折回）。
/// 下标是累计值（不是 0..N-1）：满 = `head - tail >= N`，空 = `head == tail`。
pub struct SpscRing<T, const N: usize> {
    head: Padded, // 下一个要写的位置（生产者侧）
    tail: Padded, // 下一个要读的位置（消费者侧）
    slots: UnsafeCell<[MaybeUninit<T>; N]>,
}

// SPSC 语义保证：push 只被一个线程调、pop 只被另一个线程调。
// T: Send 才能跨线程传递所有权。
unsafe impl<T: Send, const N: usize> Send for SpscRing<T, N> {}
unsafe impl<T: Send, const N: usize> Sync for SpscRing<T, N> {}

impl<T, const N: usize> SpscRing<T, N> {
    const MASK: usize = N - 1;

    pub fn new() -> Self {
        const { assert!(N.is_power_of_two(), "capacity must be power of 2") }
        Self {
            head: Padded::new(0),
            tail: Padded::new(0),
            slots: UnsafeCell::new([const { MaybeUninit::uninit() }; N]),
        }
    }

    pub const fn capacity() -> usize {
        N
    }

    /// 生产侧（只能一个线程调用）。满了返回 Err(原值)，调用方自旋再试（背压）。
    #[inline]
    pub fn try_push(&self, v: T) -> Result<(), T> {
        let h = self.head.0.load(Ordering::Relaxed);
        let t = self.tail.0.load(Ordering::Acquire);
        if h.wrapping_sub(t) >= N {
            return Err(v);
        }
        // SAFETY: 单生产者 → 此槽位无并发写；h-t < N → 此槽位上次已被 pop（或从未写），
        // 消费者不会读它（head 还没 release）。
        unsafe { (*self.slots.get())[h & Self::MASK].write(v) };
        self.head.0.store(h + 1, Ordering::Release);
        Ok(())
    }

    /// 消费侧（只能一个线程调用）。空返回 None。
    #[inline]
    pub fn try_pop(&self) -> Option<T> {
        let t = self.tail.0.load(Ordering::Relaxed);
        let h = self.head.0.load(Ordering::Acquire);
        if t == h {
            return None;
        }
        // SAFETY: acquire 读 head 保证槽位数据已就绪（与 push 的 release 配对）；
        // 单消费者 → 此槽位无并发读；生产者在本槽被消费前不会重写（h-t 满员检查）。
        let v = unsafe { (*self.slots.get())[t & Self::MASK].assume_init_read() };
        self.tail.0.store(t + 1, Ordering::Release);
        Some(v)
    }

    /// 近似深度（并发下只是快照，别拿来当同步依据）
    pub fn len_approx(&self) -> usize {
        let h = self.head.0.load(Ordering::Acquire);
        let t = self.tail.0.load(Ordering::Acquire);
        h.wrapping_sub(t)
    }

    pub fn is_empty_approx(&self) -> bool {
        self.len_approx() == 0
    }
}

impl<T, const N: usize> Drop for SpscRing<T, N> {
    fn drop(&mut self) {
        // 把没被消费的元素析构掉（防 T 有资源时泄漏）
        while self.try_pop().is_some() {}
    }
}

impl<T, const N: usize> Default for SpscRing<T, N> {
    fn default() -> Self {
        Self::new()
    }
}
