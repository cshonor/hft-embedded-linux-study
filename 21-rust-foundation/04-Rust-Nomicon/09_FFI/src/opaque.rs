//! 不透明 C 结构体：私有字段 + `PhantomData`。

use std::marker::PhantomData;

#[repr(C)]
pub struct OpaqueDb {
    _private: [u8; 0],
    _marker: PhantomData<(*mut u8, std::marker::PhantomPinned)>,
}

impl OpaqueDb {
    /// 仅持有 C 返回的指针，不在 Rust 侧构造内部布局。
    pub fn from_c_ptr(_ptr: *mut OpaqueDb) -> &'static OpaqueDb {
        // 演示用：生产代码须绑定真实 lifetime / 封装 Raw pointer。
        unsafe { &*(_ptr as *const OpaqueDb) }
    }
}

/// 比 `*mut c_void` 更类型安全：不同 opaque 类型不可混传。
#[repr(C)]
pub struct OpaqueSession {
    _private: (),
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn opaque_distinct_types() {
        fn assert_distinct<T, U>() {}
        assert_distinct::<OpaqueDb, OpaqueSession>();
    }
}
