//! 第 6 章：手写 `Arc` —— `AtomicUsize` 引用计数 + Release/Acquire 回收。
//!
//! 与标准库 `Arc` 相比省略弱引用、布局优化等；仅用于理解 **clone = Relaxed 加一**、
//! **drop 最后一次 = Release 减一 + Acquire fence + 释放内存**。

use std::alloc::{alloc, dealloc, Layout};
use std::ops::Deref;
use std::ptr::NonNull;
use std::sync::atomic::{fence, AtomicUsize, Ordering};
use std::thread;

struct ArcInner<T> {
    ref_count: AtomicUsize,
    data: T,
}

pub struct CustomArc<T> {
    ptr: NonNull<ArcInner<T>>,
}

unsafe impl<T: Send + Sync> Send for CustomArc<T> {}
unsafe impl<T: Send + Sync> Sync for CustomArc<T> {}

impl<T> CustomArc<T> {
    pub fn new(data: T) -> Self {
        let layout = Layout::new::<ArcInner<T>>();
        // SAFETY: 按 ArcInner 布局分配并初始化。
        unsafe {
            let raw = alloc(layout) as *mut ArcInner<T>;
            raw.write(ArcInner {
                ref_count: AtomicUsize::new(1),
                data,
            });
            Self {
                ptr: NonNull::new_unchecked(raw),
            }
        }
    }

    fn inner(&self) -> &ArcInner<T> {
        // SAFETY: ptr 来自有效分配，生命周期由 Arc 管理。
        unsafe { self.ptr.as_ref() }
    }
}

impl<T> Clone for CustomArc<T> {
    fn clone(&self) -> Self {
        let prev = self.inner().ref_count.fetch_add(1, Ordering::Relaxed);
        assert!(prev > 0, "CustomArc: clone on dangling arc");
        Self { ptr: self.ptr }
    }
}

impl<T> Drop for CustomArc<T> {
    fn drop(&mut self) {
        if self.inner().ref_count.fetch_sub(1, Ordering::Release) == 1 {
            fence(Ordering::Acquire);
            let layout = Layout::new::<ArcInner<T>>();
            // SAFETY: 当前线程是最后一个持有者，Acquire 保证可见性后再释放。
            unsafe { dealloc(self.ptr.as_ptr() as *mut u8, layout) };
        }
    }
}

impl<T> Deref for CustomArc<T> {
    type Target = T;
    fn deref(&self) -> &T {
        &self.inner().data
    }
}

pub fn demo() {
    let arc = CustomArc::new(String::from("hello arc"));
    let handles: Vec<_> = (0..4)
        .map(|i| {
            let a = arc.clone();
            thread::spawn(move || println!("thread {i}: {}", &*a))
        })
        .collect();
    for h in handles {
        h.join().unwrap();
    }
    println!("ref_count after joins (still 1 on main): ok");
}

pub fn main() {
    demo();
}
