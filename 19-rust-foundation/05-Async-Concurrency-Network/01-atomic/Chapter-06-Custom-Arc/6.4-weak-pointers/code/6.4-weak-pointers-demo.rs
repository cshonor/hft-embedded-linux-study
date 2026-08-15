//! 6.4 弱指针：独立 `weak_count`，`upgrade` 时尝试 `fetch_add` 强引用。

use std::alloc::{alloc, dealloc, Layout};
use std::ptr::NonNull;
use std::sync::atomic::{fence, AtomicUsize, Ordering};

struct ArcInner<T> {
    strong: AtomicUsize,
    weak: AtomicUsize,
    data: T,
}

pub struct CustomArc<T> {
    ptr: NonNull<ArcInner<T>>,
}

pub struct CustomWeak<T> {
    ptr: NonNull<ArcInner<T>>,
}

impl<T> CustomArc<T> {
    pub fn new(data: T) -> Self {
        let layout = Layout::new::<ArcInner<T>>();
        unsafe {
            let raw = alloc(layout) as *mut ArcInner<T>;
            raw.write(ArcInner {
                strong: AtomicUsize::new(1),
                weak: AtomicUsize::new(1),
                data,
            });
            Self {
                ptr: NonNull::new_unchecked(raw),
            }
        }
    }

    pub fn downgrade(&self) -> CustomWeak<T> {
        self.inner().weak.fetch_add(1, Ordering::Relaxed);
        CustomWeak { ptr: self.ptr }
    }

    fn inner(&self) -> &ArcInner<T> {
        unsafe { self.ptr.as_ref() }
    }
}

impl<T> CustomWeak<T> {
    pub fn upgrade(&self) -> Option<CustomArc<T>> {
        let inner = unsafe { self.ptr.as_ref() };
        loop {
            let s = inner.strong.load(Ordering::Relaxed);
            if s == 0 {
                return None;
            }
            if inner
                .strong
                .compare_exchange_weak(s, s + 1, Ordering::Relaxed, Ordering::Relaxed)
                .is_ok()
            {
                return Some(CustomArc { ptr: self.ptr });
            }
        }
    }
}

impl<T> Drop for CustomArc<T> {
    fn drop(&mut self) {
        if self.inner().strong.fetch_sub(1, Ordering::Release) == 1 {
            fence(Ordering::Acquire);
            if self.inner().weak.fetch_sub(1, Ordering::Release) == 1 {
                unsafe {
                    dealloc(self.ptr.as_ptr() as *mut u8, Layout::new::<ArcInner<T>>());
                }
            }
        }
    }
}

impl<T> Drop for CustomWeak<T> {
    fn drop(&mut self) {
        if unsafe { self.ptr.as_ref() }
            .weak
            .fetch_sub(1, Ordering::Release)
            == 1
        {
            fence(Ordering::Acquire);
            if unsafe { self.ptr.as_ref() }.strong.load(Ordering::Relaxed) == 0 {
                unsafe {
                    dealloc(self.ptr.as_ptr() as *mut u8, Layout::new::<ArcInner<T>>());
                }
            }
        }
    }
}

pub fn demo() {
    let arc = CustomArc::new(42u32);
    let weak = arc.downgrade();
    drop(arc);
    match weak.upgrade() {
        Some(a) => println!("=== Weak upgrade ok: {} ===", a.inner().data),
        None => println!("=== Weak upgrade failed (expected if dropped) ==="),
    }
}
