//! 第 7 章 · 内部可变性 —— 亲手实现 `Cell` / `RefCell`
//!
//! 对应笔记：
//!   - [7.2.1 UnsafeCell](../7.2.1-unsafecell.md) —— 唯一的「合法开洞」入口
//!   - [7.2.2 Cell](../7.2.2-cell.md)              —— 搬家式读写，运行期零检查
//!   - [7.3 RefCell](../7.3-refcell-overview.md)   —— 借用计数，把借用检查挪到运行期
//!   - 第 11 章前置：每个锁里的 `data` 字段本质上都是 `UnsafeCell`
//!
//! # 三段递进
//!
//! | 类型 | 手段 | 借用检查时机 | 代价 |
//! |---|---|---|---|
//! | [`MiniCell`] | 整体搬进搬出 | 无（靠类型系统禁止别名） | 拿不出 `&T` |
//! | [`MiniRefCell`] | `isize` 借用计数 | 运行期 | 每次借读写一个计数 + 失败分支 |
//! | `std::cell::UnsafeCell` | 裸开洞 | 你自己负责 | UB 风险 |
//!
//! # 为什么 `MiniCell<T>` 自动不是 `Sync`
//!
//! 它内含 `UnsafeCell<T>`，而 std 里 `UnsafeCell` 是 `!Sync`。
//! 「含 `!Sync` 字段 ⇒ 自身 `!Sync`」由编译器自动推导，**不需要** `negative impl`。
//! 这就是标准库只写一个 `UnsafeCell` 就能把 `Cell`/`RefCell` 钉在单线程上的原因。

use std::cell::UnsafeCell;
use std::ops::{Deref, DerefMut};

// ────────────────────────────── MiniCell ──────────────────────────────

/// 最小 `Cell`：只能整体搬进搬出，因此永远不会同时存在 `&T` 和 `&mut T`。
pub struct MiniCell<T> {
    value: UnsafeCell<T>,
}

impl<T> MiniCell<T> {
    pub const fn new(value: T) -> Self {
        MiniCell { value: UnsafeCell::new(value) }
    }

    /// 交出所有权（编译期即可证明此时无别名）。
    pub fn into_inner(self) -> T {
        self.value.into_inner()
    }

    /// 放入新值、返回旧值。**旧值在此刻已经离开槽位**，由调用方决定何时析构。
    pub fn replace(&self, value: T) -> T {
        // SAFETY: 调用方持有 `&self` 且 `MiniCell` 是 `!Sync`，
        // 同一时刻不会有第二个线程访问这个槽位。
        unsafe { std::mem::replace(&mut *self.value.get(), value) }
    }

    /// `set` = 先写入新值，**再**析构旧值。
    ///
    /// 顺序很关键：旧值的 `Drop::drop` 里若再次访问本 Cell（重入），
    /// 它看到的是**已经写好的新值**，而不是「半新半旧」的中间态。
    pub fn set(&self, value: T) {
        let old = self.replace(value);
        drop(old);
    }

    /// 只有 `T: Copy` 才能「读」—— 因为读出来的是一份位拷贝，
    /// 不会与原槽位产生别名。
    pub fn get(&self) -> T
    where
        T: Copy,
    {
        unsafe { *self.value.get() }
    }

    pub fn take(&self) -> T
    where
        T: Default,
    {
        self.replace(T::default())
    }

    /// 有 `&mut self` 时不需要任何 unsafe：编译期已保证独占。
    pub fn get_mut(&mut self) -> &mut T {
        self.value.get_mut()
    }

    pub fn as_ptr(&self) -> *mut T {
        self.value.get()
    }
}

// ──────────────────────────── 借用错误类型 ────────────────────────────

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct BorrowError;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct BorrowMutError;

// ──────────────────────────── MiniRefCell ────────────────────────────

/// 借用标志（与 std 同构）：
///   `0`   → 无人借用
///   `n>0` → 有 n 个共享借用
///   `-1`  → 有 1 个独占借用
pub struct MiniRefCell<T: ?Sized> {
    borrow: MiniCell<isize>,
    value: UnsafeCell<T>,
}

impl<T> MiniRefCell<T> {
    pub const fn new(value: T) -> Self {
        MiniRefCell { borrow: MiniCell::new(0), value: UnsafeCell::new(value) }
    }

    pub fn into_inner(self) -> T {
        debug_assert_eq!(self.borrow.get(), 0, "仍有活着的借用时不能 into_inner");
        self.value.into_inner()
    }
}

impl<T: ?Sized> MiniRefCell<T> {
    /// 共享借用：只要当下不是独占借用就能成功，可叠加。
    pub fn try_borrow(&self) -> Result<MiniRef<'_, T>, BorrowError> {
        let flag = self.borrow.get();
        if flag < 0 {
            return Err(BorrowError);
        }
        if flag == isize::MAX {
            return Err(BorrowError); // 共享借用溢出，std 同样拒绝
        }
        self.borrow.set(flag + 1);
        Ok(MiniRef { flag: &self.borrow, value: self.value.get() })
    }

    pub fn borrow(&self) -> MiniRef<'_, T> {
        self.try_borrow().expect("已被独占借用，不能再共享借用")
    }

    /// 独占借用：要求计数归零（既无共享也无独占）。
    pub fn try_borrow_mut(&self) -> Result<MiniRefMut<'_, T>, BorrowMutError> {
        if self.borrow.get() != 0 {
            return Err(BorrowMutError);
        }
        self.borrow.set(-1);
        Ok(MiniRefMut { flag: &self.borrow, value: self.value.get() })
    }

    pub fn borrow_mut(&self) -> MiniRefMut<'_, T> {
        self.try_borrow_mut().expect("已被借用，不能再独占借用")
    }

    /// `&mut self` ⇒ 编译期已独占，跳过运行期计数（零开销逃生口）。
    pub fn get_mut(&mut self) -> &mut T {
        self.value.get_mut()
    }

    /// 当前借用计数，仅供测试/调试观察。
    pub fn borrow_flag(&self) -> isize {
        self.borrow.get()
    }
}

pub struct MiniRef<'b, T: ?Sized> {
    flag: &'b MiniCell<isize>,
    value: *const T,
}

impl<'b, T: ?Sized> Deref for MiniRef<'b, T> {
    type Target = T;
    fn deref(&self) -> &T {
        unsafe { &*self.value }
    }
}

impl<'b, T: ?Sized> Drop for MiniRef<'b, T> {
    fn drop(&mut self) {
        let f = self.flag.get();
        debug_assert!(f > 0, "共享借用计数异常: {f}");
        self.flag.set(f - 1);
    }
}

pub struct MiniRefMut<'b, T: ?Sized> {
    flag: &'b MiniCell<isize>,
    value: *mut T,
}

impl<'b, T: ?Sized> Deref for MiniRefMut<'b, T> {
    type Target = T;
    fn deref(&self) -> &T {
        unsafe { &*self.value }
    }
}

impl<'b, T: ?Sized> DerefMut for MiniRefMut<'b, T> {
    fn deref_mut(&mut self) -> &mut T {
        unsafe { &mut *self.value }
    }
}

impl<'b, T: ?Sized> Drop for MiniRefMut<'b, T> {
    fn drop(&mut self) {
        let f = self.flag.get();
        debug_assert_eq!(f, -1, "独占借用计数异常: {f}");
        self.flag.set(0);
    }
}

// ──────────────────────────────── 测试 ────────────────────────────────

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn cell_set_get_replace_take() {
        let c = MiniCell::new(10);
        assert_eq!(c.get(), 10);
        c.set(20);
        assert_eq!(c.get(), 20);
        assert_eq!(c.replace(30), 20);
        assert_eq!(c.get(), 30);
        assert_eq!(c.take(), 30);
        assert_eq!(c.get(), 0);
    }

    /// 记录析构次数的探针，用来验证 `set` 对旧值的处理。
    struct Counted<'a> {
        drops: &'a MiniCell<usize>,
    }
    impl Drop for Counted<'_> {
        fn drop(&mut self) {
            let n = self.drops.get();
            self.drops.set(n + 1);
        }
    }

    #[test]
    fn cell_set_drops_old_exactly_once() {
        let drops = MiniCell::new(0usize);
        let holder = MiniCell::new(Counted { drops: &drops });
        holder.set(Counted { drops: &drops }); // 覆盖 ⇒ 旧值析构一次
        assert_eq!(drops.get(), 1);
        drop(holder); // 新值再析构一次
        assert_eq!(drops.get(), 2);
    }

    /// 旧值析构时，新值**已经**写进槽位 —— 这是 `set` 与
    /// 「先 drop 旧值再写入」的关键差别（后者会露出中间态）。
    struct Observe<'a> {
        slot: &'a MiniCell<Option<&'static str>>,
    }
    impl<'a> Observe<'a> {
        fn new(slot: &'a MiniCell<Option<&'static str>>, me: &'static str) -> Observe<'a> {
            slot.set(Some(me));
            Observe { slot }
        }
    }
    impl Drop for Observe<'_> {
        fn drop(&mut self) {
            let next = match self.slot.get() {
                Some("new") => Some("new+old"),
                _ => Some("unexpected"),
            };
            self.slot.set(next);
        }
    }

    #[test]
    fn cell_old_value_sees_new_value_during_drop() {
        let slot: MiniCell<Option<&'static str>> = MiniCell::new(None);
        let holder = MiniCell::new(Observe::new(&slot, "old"));
        assert_eq!(slot.get(), Some("old"));

        // 实参先求值（写 "new"）→ 再 replace → 再 drop 旧值
        holder.set(Observe::new(&slot, "new"));
        assert_eq!(slot.get(), Some("new+old"));
    }

    #[test]
    fn refcell_allows_many_shared_borrows() {
        let rc = MiniRefCell::new(5);
        let a = rc.borrow();
        let b = rc.borrow();
        assert_eq!(*a + *b, 10);
        assert_eq!(rc.borrow_flag(), 2);
        drop(a);
        assert_eq!(rc.borrow_flag(), 1);
        drop(b);
        assert_eq!(rc.borrow_flag(), 0);
    }

    #[test]
    fn refcell_try_borrow_mut_fails_while_shared() {
        let rc = MiniRefCell::new(5);
        let _a = rc.borrow();
        assert!(matches!(rc.try_borrow_mut(), Err(BorrowMutError)));
        assert!(rc.try_borrow().is_ok());
    }

    #[test]
    fn refcell_try_borrow_fails_while_mut() {
        let rc = MiniRefCell::new(5);
        let mut m = rc.borrow_mut();
        *m = 7;
        assert!(matches!(rc.try_borrow(), Err(BorrowError)));
        assert!(matches!(rc.try_borrow_mut(), Err(BorrowMutError)));
        drop(m);
        assert_eq!(*rc.borrow(), 7);
    }

    #[test]
    #[should_panic(expected = "已被独占借用")]
    fn refcell_borrow_panics_while_mutably_borrowed() {
        let rc = MiniRefCell::new(5);
        let _m = rc.borrow_mut();
        let _boom = rc.borrow();
    }

    #[test]
    fn refcell_flag_returns_to_zero_after_failed_borrow() {
        let rc = MiniRefCell::new(String::from("hi"));
        {
            let _m = rc.borrow_mut();
            assert_eq!(rc.borrow_flag(), -1);
            drop(rc.try_borrow_mut()); // 失败，不应改动计数
            assert_eq!(rc.borrow_flag(), -1);
        }
        assert_eq!(rc.borrow_flag(), 0);
        assert_eq!(&*rc.borrow(), "hi");
    }

    #[test]
    fn refcell_get_mut_skips_runtime_check() {
        let mut rc = MiniRefCell::new(1);
        *rc.get_mut() = 42;
        assert_eq!(rc.borrow_flag(), 0, "get_mut 完全不碰借用计数");
        assert_eq!(*rc.borrow(), 42);
    }

    /// 内部可变性存在的理由：`Rc` 的多个所有者都要能改同一份数据。
    ///
    /// `Rc` 只给 `&T`，本来改不了；套一层 `MiniRefCell` 就能改了，
    /// 代价是借用检查从编译期挪到运行期。std 里就是 `Rc<RefCell<T>>`。
    #[test]
    fn rc_refcell_is_the_classic_combo() {
        use std::rc::Rc;

        let shared = Rc::new(MiniRefCell::new(Vec::<i32>::new()));
        let a = Rc::clone(&shared);
        let b = Rc::clone(&shared);

        a.borrow_mut().push(1);
        b.borrow_mut().push(2);

        // 借用结束后只读，可以同时持有多个共享借用
        let r1 = shared.borrow();
        let r2 = shared.borrow();
        assert_eq!(*r1, vec![1, 2]);
        assert_eq!(*r2, vec![1, 2]);
        assert_eq!(shared.borrow_flag(), 2);
    }
}
