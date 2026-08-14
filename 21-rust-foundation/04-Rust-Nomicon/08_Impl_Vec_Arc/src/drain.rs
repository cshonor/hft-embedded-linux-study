//! `Drain`：迭代移除子区间。创建时 `vec.len = 0`（Nomicon 泄漏放大防护），Drop 时 compact。

use crate::my_vec::MyVec;
use crate::zst;
use std::marker::PhantomData;
use std::ops::Range;
use std::ptr;

pub struct Drain<'a, T> {
    vec: &'a mut MyVec<T>,
    range_start: usize,
    iter: usize,
    drain_end: usize,
    tail_len: usize,
    _marker: PhantomData<T>,
}

impl<'a, T> Drain<'a, T> {
    pub fn new(vec: &'a mut MyVec<T>, range: Range<usize>) -> Self {
        assert!(range.start <= range.end && range.end <= vec.len());
        let tail_len = vec.len - range.end;
        let drain_end = range.end;
        let range_start = range.start;
        vec.len = 0;
        Drain {
            vec,
            range_start,
            iter: range_start,
            drain_end,
            tail_len,
            _marker: PhantomData,
        }
    }
}

impl<'a, T> Iterator for Drain<'a, T> {
    type Item = T;

    fn next(&mut self) -> Option<T> {
        if self.iter >= self.drain_end {
            return None;
        }
        let elem = if zst::is_zst::<T>() {
            zst::zst_read()
        } else {
            unsafe {
                self.vec
                    .buf
                    .ptr
                    .as_ptr()
                    .add(self.iter)
                    .read()
            }
        };
        self.iter += 1;
        Some(elem)
    }
}

impl<'a, T> Drop for Drain<'a, T> {
    fn drop(&mut self) {
        let drained_count = self.iter - self.range_start;
        let new_len = self.range_start + self.tail_len;
        if !zst::is_zst::<T>() && self.tail_len > 0 {
            unsafe {
                ptr::copy(
                    self.vec.buf.ptr.as_ptr().add(self.drain_end),
                    self.vec.buf.ptr.as_ptr().add(self.range_start),
                    self.tail_len,
                );
            }
        }
        let _ = drained_count;
        self.vec.len = new_len;
    }
}

#[cfg(test)]
mod tests {
    use crate::my_vec::MyVec;

    #[test]
    fn drain_middle() {
        let mut v = MyVec::from_iter(vec![1, 2, 3, 4]);
        let drained: Vec<_> = v.drain(1..3).collect();
        assert_eq!(drained, vec![2, 3]);
        assert_eq!(&v[..], &[1, 4]);
    }
}
