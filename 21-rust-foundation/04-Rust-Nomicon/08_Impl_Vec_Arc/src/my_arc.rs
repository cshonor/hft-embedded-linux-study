//! Nomicon 风格简化 `Arc`（无 `Weak`）。

use std::marker::PhantomData;
use std::ops::Deref;
use std::process;
use std::ptr::NonNull;
use std::sync::atomic::{self, AtomicUsize, Ordering};

struct ArcInner<T> {
    rc: AtomicUsize,
    data: T,
}

pub struct MyArc<T> {
    ptr: NonNull<ArcInner<T>>,
    phantom: PhantomData<ArcInner<T>>,
}

impl<T> MyArc<T> {
    pub fn new(data: T) -> Self {
        let inner = Box::new(ArcInner {
            rc: AtomicUsize::new(1),
            data,
        });
        MyArc {
            ptr: NonNull::new(Box::into_raw(inner)).expect("Box into_raw non-null"),
            phantom: PhantomData,
        }
    }

    fn inner(&self) -> &ArcInner<T> {
        unsafe { self.ptr.as_ref() }
    }

    pub fn strong_count(&self) -> usize {
        self.inner().rc.load(Ordering::Relaxed)
    }
}

unsafe impl<T: Send + Sync> Send for MyArc<T> {}
unsafe impl<T: Send + Sync> Sync for MyArc<T> {}

impl<T> Deref for MyArc<T> {
    type Target = T;

    fn deref(&self) -> &T {
        &self.inner().data
    }
}

impl<T> Clone for MyArc<T> {
    fn clone(&self) -> Self {
        let old = self.inner().rc.fetch_add(1, Ordering::Relaxed);
        if old > isize::MAX as usize {
            process::abort();
        }
        MyArc {
            ptr: self.ptr,
            phantom: PhantomData,
        }
    }
}

impl<T> Drop for MyArc<T> {
    fn drop(&mut self) {
        if self.inner().rc.fetch_sub(1, Ordering::Release) != 1 {
            return;
        }
        atomic::fence(Ordering::Acquire);
        unsafe {
            drop(Box::from_raw(self.ptr.as_ptr()));
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::mpsc;
    use std::thread;

    #[test]
    fn clone_shares_data() {
        let a = MyArc::new(42);
        let b = a.clone();
        assert_eq!(*a, 42);
        assert_eq!(*b, 42);
        assert_eq!(a.strong_count(), 2);
    }

    #[test]
    fn drops_when_last() {
        let a = MyArc::new(String::from("x"));
        let b = a.clone();
        drop(b);
        assert_eq!(a.strong_count(), 1);
    }

    #[test]
    fn send_across_threads() {
        let arc = MyArc::new(vec![1, 2, 3]);
        let arc2 = arc.clone();
        let (tx, rx) = mpsc::channel();
        thread::spawn(move || {
            tx.send(arc2[0]).unwrap();
        });
        assert_eq!(rx.recv().unwrap(), 1);
        assert_eq!(arc[1], 2);
    }
}
