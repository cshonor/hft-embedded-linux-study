//! 消费型迭代器：双指针 + `ptr::read`。

use crate::my_vec::MyVec;
use crate::raw_vec::RawVec;
use crate::zst;
use std::mem::ManuallyDrop;
use std::ptr;

pub struct IntoIter<T> {
    buf: ManuallyDrop<RawVec<T>>,
    start: *const T,
    end: *const T,
}

impl<T> IntoIter<T> {
    pub fn new(mut vec: MyVec<T>) -> Self {
        let start = vec.buf.ptr.as_ptr();
        let end = if zst::is_zst::<T>() {
            zst::zst_ptr_add(start, vec.len)
        } else {
            unsafe { start.add(vec.len) }
        };
        let buf = ManuallyDrop::new(RawVec {
            ptr: vec.buf.ptr,
            cap: vec.buf.cap,
        });
        vec.buf.cap = 0;
        vec.len = 0;
        IntoIter { buf, start, end }
    }
}

impl<T> Iterator for IntoIter<T> {
    type Item = T;

    fn next(&mut self) -> Option<T> {
        if self.start == self.end {
            return None;
        }
        let elem = if zst::is_zst::<T>() {
            zst::zst_read()
        } else {
            unsafe { ptr::read(self.start) }
        };
        self.start = if zst::is_zst::<T>() {
            zst::zst_ptr_add(self.start, 1)
        } else {
            unsafe { self.start.add(1) }
        };
        Some(elem)
    }
}

impl<T> Drop for IntoIter<T> {
    fn drop(&mut self) {
        while self.next().is_some() {}
        unsafe {
            ManuallyDrop::drop(&mut self.buf);
        }
    }
}

#[cfg(test)]
mod tests {
    use crate::my_vec::MyVec;

    #[test]
    fn into_iter_sum() {
        let v = MyVec::from_iter(vec![1, 2, 3]);
        let s: i32 = v.into_iter().sum();
        assert_eq!(s, 6);
    }
}
