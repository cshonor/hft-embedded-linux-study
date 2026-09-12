//! 第 11 章 · 并发 —— 亲手实现三件套
//!
//! 对应笔记：
//!   - [11.1 Futex](../11.1-futex.md) —— 真实 std 在竞争路径上**睡进内核**
//!   - [11.2 Mutex](../11.2-mutex-overview.md) —— 锁 = `AtomicUsize` 状态机 + `UnsafeCell` 数据
//!   - [11.6 Once](../11.6-once.md)    —— 三态状态机，一次初始化
//!   - [11.10 MPSC](../11.10-mpsc-overview.md) —— 消息通道
//!
//! # 为什么这里没有「真正的 futex」
//!
//! futex 是 Linux 系统调用，Windows 对应 `WaitOnAddress`，std 把它们收在
//! `sys::sync` 里（见笔记的「三层架构」）。本 lab 要跨 Windows/macOS/Linux 都能跑，
//! 所以 `MiniMutex` 用 **自旋 + 退让** 近似竞争路径：
//!
//! | | 真实 std | 本 lab |
//! |---|---|---|
//! | 无竞争 | 一条 CAS，零 syscall | 一条 CAS |
//! | 有竞争 | `futex(FUTEX_WAIT)` 睡眠 | `spin_loop` → `yield_now` |
//!
//! 结论：**快路径的语义完全一致，慢路径只是「不睡觉」**，对理解状态机没有影响。
//!
//! # 三件东西
//!
//! | 类型 | 对应 std | 看点 |
//! |---|---|---|
//! | [`SpscRing`] | （std 没有，HFT 自己造） | 无锁、2 的幂掩码取模、cacheline 隔离 |
//! | [`MiniMutex`] | `std::sync::Mutex` | `UnsafeCell` + 状态机 + RAII guard |
//! | [`MiniOnce`] / [`MiniOnceLock`] | `Once` / `OnceLock` | 三态 + Acquire/Release 配对 |

use std::alloc::{alloc, dealloc, Layout};
use std::cell::UnsafeCell;
use std::hint::spin_loop;
use std::marker::PhantomData;
use std::mem::MaybeUninit;
use std::ops::{Deref, DerefMut};
use std::ptr;
use std::sync::atomic::{AtomicBool, AtomicU8, AtomicUsize, Ordering};

// ══════════════════════════ 0. cacheline 隔离 ══════════════════════════

/// 把热点计数器各自放到独立 cacheline，避免 false sharing。
///
/// 在 SPSC 队列里 `head`（消费者写）和 `tail`（生产者写）如果落在同一条
/// cacheline，两侧会互相把对方的 line 置为 Invalid —— 这就是伪共享。
#[repr(align(64))]
struct CachePadded<T> {
    value: T,
}

impl<T> Deref for CachePadded<T> {
    type Target = T;
    fn deref(&self) -> &T {
        &self.value
    }
}

// ═════════════════════ 1. 无锁 SPSC 环形队列 ═════════════════════

/// 单生产者 / 单消费者无锁环形队列。
///
/// # 并发契约（由调用方保证，这正是 SPSC 的前提）
/// - **同一时刻只有一个线程**调用 [`push`](Self::push)；
/// - **同一时刻只有一个线程**调用 [`pop`](Self::pop)。
///
/// 在这个前提下 `push`/`pop` 才敢只用一个 `Relaxed` 读自己的下标、
/// `Acquire` 读对方下标、`Release` 发布自己的下标。
pub struct SpscRing<T> {
    ptr: *mut T,
    cap: usize,
    mask: usize,
    /// 消费者推进（生产者 Acquire 读）
    head: CachePadded<AtomicUsize>,
    /// 生产者推进（消费者 Acquire 读）
    tail: CachePadded<AtomicUsize>,
    _marker: PhantomData<T>,
}

// T 能跨线程搬动 ⇒ 队列可以被两个线程各用一半。
unsafe impl<T: Send> Send for SpscRing<T> {}
unsafe impl<T: Send> Sync for SpscRing<T> {}

impl<T> SpscRing<T> {
    /// 容量必须是 **2 的幂**：用 `idx & (cap - 1)` 代替 `idx % cap`，
    /// 省掉一次除法（在每秒百万次的行情通路里这是实打实的收益）。
    pub fn new(cap: usize) -> Self {
        assert!(cap >= 2, "容量至少为 2，实际 {cap}");
        assert!(cap.is_power_of_two(), "容量必须是 2 的幂，实际 {cap}");

        let layout = Layout::array::<T>(cap).expect("容量过大，Layout 溢出");
        let ptr = if layout.size() == 0 {
            // ZST 不需要真实分配，给一个对齐良好的悬垂指针即可
            ptr::NonNull::<T>::dangling().as_ptr()
        } else {
            let raw = unsafe { alloc(layout) } as *mut T;
            assert!(!raw.is_null(), "SpscRing 内存分配失败");
            raw
        };

        SpscRing {
            ptr,
            cap,
            mask: cap - 1,
            head: CachePadded { value: AtomicUsize::new(0) },
            tail: CachePadded { value: AtomicUsize::new(0) },
            _marker: PhantomData,
        }
    }

    pub fn capacity(&self) -> usize {
        self.cap
    }

    pub fn len(&self) -> usize {
        self.tail.load(Ordering::Acquire).wrapping_sub(self.head.load(Ordering::Acquire))
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    /// 生产者专用。队列满时把值**原样退回**（`Err(value)`），不丢数据。
    pub fn push(&self, value: T) -> Result<(), T> {
        let tail = self.tail.load(Ordering::Relaxed); // 只有我写 tail
        let head = self.head.load(Ordering::Acquire); // 要看到消费者释放出来的槽位

        if tail.wrapping_sub(head) == self.cap {
            return Err(value);
        }

        // SAFETY: 该槽位消费者已释放（Acquire 读到），且只有我写这个下标
        unsafe { ptr::write(self.ptr.add(tail & self.mask), value) };

        // Release：把「写入的元素」发布给消费者
        self.tail.store(tail.wrapping_add(1), Ordering::Release);
        Ok(())
    }

    /// 消费者专用。
    pub fn pop(&self) -> Option<T> {
        let head = self.head.load(Ordering::Relaxed); // 只有我写 head
        let tail = self.tail.load(Ordering::Acquire); // 要看到生产者发布的元素

        if head == tail {
            return None;
        }

        // SAFETY: tail > head 说明该槽位已被生产者写满，且只有我读这个下标
        let v = unsafe { ptr::read(self.ptr.add(head & self.mask)) };

        // Release：把「槽位已空闲」发布给生产者
        self.head.store(head.wrapping_add(1), Ordering::Release);
        Some(v)
    }
}

impl<T> Drop for SpscRing<T> {
    fn drop(&mut self) {
        // 先把没取走的元素析构掉，再释放裸内存，否则会泄漏
        while self.pop().is_some() {}
        let layout = Layout::array::<T>(self.cap).unwrap();
        if layout.size() != 0 {
            unsafe { dealloc(self.ptr as *mut u8, layout) };
        }
    }
}

// ═════════════════════════ 2. 最小 Mutex ═════════════════════════

/// 最小互斥锁：`AtomicBool` 状态 + `UnsafeCell` 数据 + RAII guard。
///
/// 与 std 的差异：竞争时**自旋退让**而非 futex 睡眠；且**没有中毒（poison）处理**。
pub struct MiniMutex<T> {
    locked: AtomicBool,
    data: UnsafeCell<T>,
}

// T 能跨线程搬动 ⇒ 锁可以在线程间共享（这就是 `Mutex<T>: Sync` 的由来）
unsafe impl<T: Send> Send for MiniMutex<T> {}
unsafe impl<T: Send> Sync for MiniMutex<T> {}

pub struct MiniMutexGuard<'a, T> {
    mutex: &'a MiniMutex<T>,
    // std 的 MutexGuard 是 !Send（用 negative impl 声明，稳定版拿不到）。
    // 这里塞一个 `Rc<()>` 的 PhantomData：Rc 不是 Send，于是 guard 自动 !Send + !Sync。
    _no_send: PhantomData<std::rc::Rc<()>>,
}

impl<T> MiniMutex<T> {
    pub const fn new(value: T) -> Self {
        MiniMutex { locked: AtomicBool::new(false), data: UnsafeCell::new(value) }
    }

    pub fn lock(&self) -> MiniMutexGuard<'_, T> {
        // 快路径：无竞争时一条 CAS 搞定，零系统调用（futex 的核心卖点）
        if self
            .locked
            .compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed)
            .is_ok()
        {
            return MiniMutexGuard { mutex: self, _no_send: PhantomData };
        }
        self.lock_slow();
        MiniMutexGuard { mutex: self, _no_send: PhantomData }
    }

    #[cold]
    fn lock_slow(&self) {
        let mut spins = 0u32;
        loop {
            if self
                .locked
                .compare_exchange_weak(false, true, Ordering::Acquire, Ordering::Relaxed)
                .is_ok()
            {
                return;
            }
            spins += 1;
            if spins < 64 {
                spin_loop(); // 短暂自旋：持锁者很可能马上就放
            } else {
                std::thread::yield_now(); // 再让出 CPU（真实 std 在此处 futex 睡眠）
                spins = 0;
            }
        }
    }

    pub fn try_lock(&self) -> Option<MiniMutexGuard<'_, T>> {
        self.locked
            .compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed)
            .ok()
            .map(|_| MiniMutexGuard { mutex: self, _no_send: PhantomData })
    }

    pub fn into_inner(self) -> T {
        self.data.into_inner()
    }
}

impl<'a, T> Deref for MiniMutexGuard<'a, T> {
    type Target = T;
    fn deref(&self) -> &T {
        // SAFETY: 持有 guard ⇒ 已 Acquire 到锁 ⇒ 独占访问成立
        unsafe { &*self.mutex.data.get() }
    }
}

impl<'a, T> DerefMut for MiniMutexGuard<'a, T> {
    fn deref_mut(&mut self) -> &mut T {
        unsafe { &mut *self.mutex.data.get() }
    }
}

impl<'a, T> Drop for MiniMutexGuard<'a, T> {
    fn drop(&mut self) {
        // Release：把「临界区内的所有写」发布给下一个 Acquire 到锁的线程
        self.mutex.locked.store(false, Ordering::Release);
    }
}

// ══════════════════════ 3. Once / OnceLock ══════════════════════

const INCOMPLETE: u8 = 0;
const RUNNING: u8 = 1;
const COMPLETE: u8 = 2;

/// 三态状态机：未完成 → 进行中 → 完成。
///
/// 注意：本实现**没有中毒处理**。若闭包 panic，状态会停在 `RUNNING`，
/// 后续等待者会一直自旋（std 则是标记「中毒」并让后续调用 panic）。
pub struct MiniOnce {
    state: AtomicU8,
}

impl MiniOnce {
    pub const fn new() -> Self {
        MiniOnce { state: AtomicU8::new(INCOMPLETE) }
    }

    pub fn is_completed(&self) -> bool {
        self.state.load(Ordering::Acquire) == COMPLETE
    }

    pub fn call_once<F: FnOnce()>(&self, f: F) {
        match self.state.compare_exchange(
            INCOMPLETE,
            RUNNING,
            Ordering::Acquire,
            Ordering::Relaxed,
        ) {
            Ok(_) => {
                f();
                // Release：把闭包里的所有写发布给等待者
                self.state.store(COMPLETE, Ordering::Release);
            }
            Err(_) => {
                // 别人在跑（或已跑完）：自旋等到 COMPLETE
                while self.state.load(Ordering::Acquire) != COMPLETE {
                    spin_loop();
                }
            }
        }
    }
}

impl Default for MiniOnce {
    fn default() -> Self {
        Self::new()
    }
}

/// `MiniOnce` + `UnsafeCell<MaybeUninit<T>>` = 一次性初始化的共享值。
pub struct MiniOnceLock<T> {
    once: MiniOnce,
    value: UnsafeCell<MaybeUninit<T>>,
}

unsafe impl<T: Send + Sync> Sync for MiniOnceLock<T> {}

impl<T> MiniOnceLock<T> {
    pub const fn new() -> Self {
        MiniOnceLock { once: MiniOnce::new(), value: UnsafeCell::new(MaybeUninit::uninit()) }
    }

    pub fn get(&self) -> Option<&T> {
        if self.once.is_completed() {
            // Acquire 到 COMPLETE ⇒ 一定能看到初始化写入的值
            Some(unsafe { &*(self.value.get() as *const T) })
        } else {
            None
        }
    }

    pub fn set(&self, value: T) -> Result<(), T> {
        let mut slot = Some(value);
        self.once.call_once(|| {
            if let Some(v) = slot.take() {
                unsafe { ptr::write(self.value.get() as *mut T, v) };
            }
        });
        match slot {
            // 我不是第一个：闭包没跑，值原样退回
            Some(v) => Err(v),
            None => Ok(()),
        }
    }

    pub fn get_or_init<F: FnOnce() -> T>(&self, f: F) -> &T {
        if let Some(v) = self.get() {
            return v;
        }
        let mut f = Some(f);
        self.once.call_once(|| {
            let v = (f.take().unwrap())();
            unsafe { ptr::write(self.value.get() as *mut T, v) };
        });
        unsafe { &*(self.value.get() as *const T) }
    }
}

impl<T> Default for MiniOnceLock<T> {
    fn default() -> Self {
        Self::new()
    }
}

impl<T> Drop for MiniOnceLock<T> {
    fn drop(&mut self) {
        if self.once.is_completed() {
            unsafe { ptr::drop_in_place(self.value.get() as *mut T) };
        }
    }
}

// ══════════════════════════════ 测试 ══════════════════════════════

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::Arc;
    use std::thread;

    // ── SpscRing ──

    #[test]
    fn spsc_preserves_fifo_order() {
        let q: SpscRing<i32> = SpscRing::new(8);
        for i in 0..5 {
            q.push(i).unwrap();
        }
        assert_eq!(q.len(), 5);
        let got: Vec<i32> = std::iter::from_fn(|| q.pop()).collect();
        assert_eq!(got, vec![0, 1, 2, 3, 4]);
        assert!(q.is_empty());
    }

    #[test]
    fn spsc_full_returns_the_value_back() {
        let q: SpscRing<i32> = SpscRing::new(4);
        for i in 0..4 {
            q.push(i).unwrap();
        }
        // 满了：值被原样退回，不是被丢掉
        assert_eq!(q.push(99), Err(99));
        assert_eq!(q.pop(), Some(0));
        assert_eq!(q.push(99), Ok(()));
    }

    #[test]
    fn spsc_empty_returns_none() {
        let q: SpscRing<i32> = SpscRing::new(4);
        assert_eq!(q.pop(), None);
    }

    #[test]
    #[should_panic(expected = "2 的幂")]
    fn spsc_capacity_must_be_power_of_two() {
        let _q: SpscRing<i32> = SpscRing::new(6);
    }

    /// 队列析构时必须把还没取走的元素也析构掉。
    #[test]
    fn spsc_drop_drops_leftover_items() {
        let drops = Arc::new(AtomicUsize::new(0));
        struct Noisy(Arc<AtomicUsize>);
        impl Drop for Noisy {
            fn drop(&mut self) {
                self.0.fetch_add(1, Ordering::SeqCst);
            }
        }

        let q: SpscRing<Noisy> = SpscRing::new(8);
        for _ in 0..3 {
            assert!(q.push(Noisy(Arc::clone(&drops))).is_ok());
        }
        assert!(q.pop().is_some(), "应取走 1 个"); // 取走 1 个
        assert_eq!(drops.load(Ordering::SeqCst), 1);
        drop(q);
        assert_eq!(drops.load(Ordering::SeqCst), 3, "剩余 2 个应随队列一起析构");
    }

    /// 真·跨线程：生产者推 1_000_000 个，消费者全部取走并校验。
    #[test]
    fn spsc_cross_thread_transfers_everything() {
        const CAP: usize = 1 << 12;
        const N: u64 = 1_000_000;

        let q: SpscRing<u64> = SpscRing::new(CAP);
        thread::scope(|s| {
            s.spawn(|| {
                for i in 0..N {
                    while q.push(i).is_err() {
                        spin_loop(); // 队列满，等消费者
                    }
                }
            });
            s.spawn(|| {
                let mut acc = 0u64;
                let mut seen = 0u64;
                while seen < N {
                    match q.pop() {
                        Some(v) => {
                            acc += v;
                            seen += 1;
                        }
                        None => spin_loop(),
                    }
                }
                assert_eq!(seen, N);
                assert_eq!(acc, (0..N).sum::<u64>());
            });
        });
    }

    // ── MiniMutex ──

    #[test]
    fn mutex_counter_from_many_threads() {
        let m = Arc::new(MiniMutex::new(0u64));
        let threads = 8;
        let per = 10_000;
        thread::scope(|s| {
            for _ in 0..threads {
                s.spawn(|| {
                    for _ in 0..per {
                        *m.lock() += 1;
                    }
                });
            }
        });
        assert_eq!(*m.lock(), threads * per);
    }

    #[test]
    fn mutex_try_lock_fails_while_held() {
        let m = MiniMutex::new(7);
        let g = m.lock();
        assert!(m.try_lock().is_none());
        assert_eq!(*g, 7);
        drop(g);
        assert!(m.try_lock().is_some(), "释放后应能再次加锁");
    }

    #[test]
    fn mutex_into_inner_and_deref_mut() {
        let m = MiniMutex::new(String::from("a"));
        m.lock().push('b');
        assert_eq!(m.into_inner(), "ab");
    }

    // ── MiniOnce / MiniOnceLock ──

    #[test]
    fn once_runs_exactly_once_under_many_threads() {
        static ONCE: MiniOnce = MiniOnce::new();
        let runs = Arc::new(AtomicUsize::new(0));

        thread::scope(|s| {
            for _ in 0..16 {
                let r = Arc::clone(&runs);
                s.spawn(move || {
                    ONCE.call_once(|| {
                        r.fetch_add(1, Ordering::SeqCst);
                    });
                    assert!(ONCE.is_completed());
                });
            }
        });

        assert_eq!(runs.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn once_lock_get_or_init_is_idempotent() {
        let cell = MiniOnceLock::new();
        assert!(cell.get().is_none());
        let a = cell.get_or_init(|| 41);
        let b = cell.get_or_init(|| 999); // 不会再执行
        assert_eq!(*a, 41);
        assert_eq!(*b, 41);
        assert_eq!(cell.get(), Some(&41));
    }

    #[test]
    fn once_lock_set_twice_second_fails() {
        let cell: MiniOnceLock<i32> = MiniOnceLock::new();
        assert_eq!(cell.set(1), Ok(()));
        assert_eq!(cell.set(2), Err(2));
        assert_eq!(cell.get(), Some(&1));
    }
}
