//! PhantomData：逻辑上拥有 `T` / `'a`，物理字段中无存储。

use std::marker::PhantomData;

/// 裸 Vec 式句柄：ptr 不携带 `T` 的 variance 信息，须用 PhantomData。
pub struct RawBuf<T> {
    ptr: *mut u8, // 占位；真实 Vec 实现中会用于 alloc/dealloc
    len: usize,
    _marker: PhantomData<T>,
}

impl<T> RawBuf<T> {
    pub fn empty() -> Self {
        RawBuf {
            ptr: std::ptr::null_mut(),
            len: 0,
            _marker: PhantomData,
        }
    }

    pub fn is_empty_buf(&self) -> bool {
        self.ptr.is_null()
    }

    pub fn len(&self) -> usize {
        self.len
    }
}

/// 逻辑上借用了 `'a`，但字段里只有 raw ptr。
pub struct RefHolder<'a, T> {
    ptr: *const T,
    _life: PhantomData<&'a T>,
}

impl<'a, T> RefHolder<'a, T> {
    pub fn new(r: &'a T) -> Self {
        RefHolder {
            ptr: r as *const T,
            _life: PhantomData,
        }
    }

    pub fn get(&self) -> &'a T {
        unsafe { &*self.ptr }
    }
}

pub fn demo() -> (usize, bool) {
    let buf = RawBuf::<i32>::empty();
    let x = 42;
    let holder = RefHolder::new(&x);
    let _ = holder.get();
    (buf.len(), buf.is_empty_buf())
}
