//! `RawVec`：指针 + 容量；ZST 与延迟分配。

use crate::zst;
use std::alloc::{self, Layout};
use std::ptr::NonNull;

pub struct RawVec<T> {
    pub ptr: NonNull<T>,
    pub cap: usize,
}

impl<T> RawVec<T> {
    pub fn new() -> Self {
        RawVec {
            ptr: NonNull::dangling(),
            cap: if zst::is_zst::<T>() {
                zst::zst_cap()
            } else {
                0
            },
        }
    }

    fn layout(cap: usize) -> Layout {
        Layout::array::<T>(cap).expect("capacity overflow")
    }

    pub fn grow(&mut self) {
        if zst::is_zst::<T>() {
            return;
        }
        let new_cap = if self.cap == 0 { 1 } else { self.cap * 2 };
        let new_layout = Self::layout(new_cap);
        assert!(new_layout.size() <= isize::MAX as usize);

        let new_ptr = if self.cap == 0 {
            unsafe { alloc::alloc(new_layout) }
        } else {
            let old_layout = Self::layout(self.cap);
            unsafe { alloc::realloc(self.ptr.as_ptr() as *mut u8, old_layout, new_layout.size()) }
        };

        if new_ptr.is_null() {
            alloc::handle_alloc_error(new_layout);
        }

        self.ptr = NonNull::new(new_ptr as *mut T).unwrap();
        self.cap = new_cap;
    }

    pub fn dealloc_buffer(&mut self) {
        if self.cap == 0 || zst::is_zst::<T>() {
            self.cap = if zst::is_zst::<T>() {
                zst::zst_cap()
            } else {
                0
            };
            return;
        }
        let layout = Self::layout(self.cap);
        unsafe {
            alloc::dealloc(self.ptr.as_ptr() as *mut u8, layout);
        }
        self.ptr = NonNull::dangling();
        self.cap = 0;
    }
}

impl<T> Drop for RawVec<T> {
    fn drop(&mut self) {
        self.dealloc_buffer();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn empty_has_zero_cap() {
        let r: RawVec<i32> = RawVec::new();
        assert_eq!(r.cap, 0);
    }

    #[test]
    fn zst_has_max_cap() {
        struct Z;
        let r: RawVec<Z> = RawVec::new();
        assert_eq!(r.cap, usize::MAX);
    }
}
