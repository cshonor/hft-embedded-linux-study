//! Nomicon 风格 `MyVec`：NonNull + len + RawVec。

use crate::drain::Drain;
use crate::into_iter::IntoIter;
use crate::raw_vec::RawVec;
use crate::zst;
use std::mem;
use std::ops::{Deref, DerefMut};
use std::ptr;

pub struct MyVec<T> {    pub(crate) buf: RawVec<T>,
    pub(crate) len: usize,
}

unsafe impl<T: Send> Send for MyVec<T> {}
unsafe impl<T: Sync> Sync for MyVec<T> {}

impl<T> MyVec<T> {
    pub fn new() -> Self {
        MyVec {
            buf: RawVec::new(),
            len: 0,
        }
    }

    pub fn with_capacity(cap: usize) -> Self {
        let mut v = MyVec::new();
        if cap > 0 && !zst::is_zst::<T>() {
            while v.buf.cap < cap {
                v.buf.grow();
            }
        }
        v
    }

    pub fn len(&self) -> usize {
        self.len
    }

    pub fn capacity(&self) -> usize {
        self.buf.cap
    }

    pub fn is_empty(&self) -> bool {
        self.len == 0
    }

    pub fn from_iter<I: IntoIterator<Item = T>>(iter: I) -> Self {
        let mut v = MyVec::new();
        for item in iter {
            v.push(item);
        }
        v
    }

    pub fn push(&mut self, elem: T) {
        if self.len == self.buf.cap {
            self.buf.grow();
        }
        if zst::is_zst::<T>() {
            mem::forget(elem);
        } else {
            unsafe {
                self.buf.ptr.as_ptr().add(self.len).write(elem);
            }
        }
        self.len += 1;
    }

    pub fn pop(&mut self) -> Option<T> {
        if self.len == 0 {
            return None;
        }
        self.len -= 1;
        Some(if zst::is_zst::<T>() {
            zst::zst_read()
        } else {
            unsafe { self.buf.ptr.as_ptr().add(self.len).read() }
        })
    }

    pub fn insert(&mut self, index: usize, elem: T) {
        assert!(index <= self.len);
        if self.len == self.buf.cap {
            self.buf.grow();
        }
        if zst::is_zst::<T>() {
            mem::forget(elem);
        } else {
            unsafe {
                let ptr = self.buf.ptr.as_ptr().add(index);
                ptr::copy(ptr, ptr.add(1), self.len - index);
                ptr.write(elem);
            }
        }
        self.len += 1;
    }

    pub fn remove(&mut self, index: usize) -> T {
        assert!(index < self.len);
        let elem = if zst::is_zst::<T>() {
            zst::zst_read()
        } else {
            unsafe {
                let ptr = self.buf.ptr.as_ptr().add(index);
                let e = ptr.read();
                ptr::copy(ptr.add(1), ptr, self.len - index - 1);
                e
            }
        };
        self.len -= 1;
        elem
    }

    pub fn drain(&mut self, range: std::ops::Range<usize>) -> Drain<'_, T> {
        Drain::new(self, range)
    }
}

impl<T> Drop for MyVec<T> {
    fn drop(&mut self) {
        while self.pop().is_some() {}
    }
}

impl<T> Deref for MyVec<T> {
    type Target = [T];
    fn deref(&self) -> &[T] {
        unsafe { std::slice::from_raw_parts(self.buf.ptr.as_ptr(), self.len) }
    }
}

impl<T> DerefMut for MyVec<T> {
    fn deref_mut(&mut self) -> &mut [T] {
        unsafe { std::slice::from_raw_parts_mut(self.buf.ptr.as_ptr(), self.len) }
    }
}

impl<T> IntoIterator for MyVec<T> {
    type Item = T;
    type IntoIter = IntoIter<T>;

    fn into_iter(self) -> IntoIter<T> {
        IntoIter::new(self)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn push_pop_deref() {
        let mut v = MyVec::new();
        v.push(1);
        v.push(2);
        assert_eq!(v.len(), 2);
        assert_eq!(v[0], 1);
        assert_eq!(v.pop(), Some(2));
        assert_eq!(v.pop(), Some(1));
        assert!(v.is_empty());
    }

    #[test]
    fn insert_remove() {
        let mut v = MyVec::from_iter(vec![1, 2, 4]);
        v.insert(2, 3);
        assert_eq!(&v[..], &[1, 2, 3, 4]);
        assert_eq!(v.remove(1), 2);
        assert_eq!(&v[..], &[1, 3, 4]);
    }

    #[test]
    fn zst_vec() {
        struct Z;
        let mut v: MyVec<Z> = MyVec::new();
        v.push(Z);
        v.push(Z);
        assert_eq!(v.len(), 2);
        assert_eq!(v.pop().is_some(), true);
    }
}
