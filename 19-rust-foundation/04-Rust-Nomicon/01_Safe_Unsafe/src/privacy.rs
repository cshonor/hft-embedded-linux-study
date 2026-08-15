//! Nomicon ch1：用模块可见性封装 invariant，对外 Safe API。

/// 最小缓冲区：`len`/`cap`/`ptr` 私有，外部无法破坏 unsafe 前提。
pub struct MiniBuf {
    ptr: *mut u8,
    len: usize,
    cap: usize,
}

impl MiniBuf {
    pub fn with_capacity(cap: usize) -> Self {
        assert!(cap > 0);
        let layout = std::alloc::Layout::array::<u8>(cap).unwrap();
        let ptr = unsafe { std::alloc::alloc(layout) };
        if ptr.is_null() {
            std::alloc::handle_alloc_error(layout);
        }
        MiniBuf {
            ptr,
            len: 0,
            cap,
        }
    }

    pub fn push(&mut self, byte: u8) {
        assert!(self.len < self.cap);
        unsafe {
            self.ptr.add(self.len).write(byte);
        }
        self.len += 1;
    }

    pub fn as_slice(&self) -> &[u8] {
        unsafe { std::slice::from_raw_parts(self.ptr, self.len) }
    }
}

impl Drop for MiniBuf {
    fn drop(&mut self) {
        if self.cap > 0 {
            let layout = std::alloc::Layout::array::<u8>(self.cap).unwrap();
            unsafe {
                std::alloc::dealloc(self.ptr, layout);
            }
        }
    }
}
