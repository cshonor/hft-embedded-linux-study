//! Item 16：Miri CI 样例 —— unsafe 封装在 crate 内，对外 safe API。
//!
//! CI：`cargo +nightly miri test -p item-16-miri`
//!
//! Strict Provenance 违规示例见模块 [`strict_provenance_bad`] 的文档（勿在 CI 中运行）。

/// 交换两个非重叠的可变引用；Miri 会在重叠引用时报 UB。
pub fn swap_if_different(a: &mut u32, b: &mut u32) {
    assert!(
        !std::ptr::eq(a as *mut u32, b as *mut u32),
        "overlapping references are UB under Miri"
    );
    std::mem::swap(a, b);
}

/// `Box::into_raw` / `from_raw` 往返 —— 与 Item 34 FFI 边界同一模式。
pub struct HeapTag {
    pub tag: u32,
}

pub fn export_tag(value: u32) -> *mut HeapTag {
    Box::into_raw(Box::new(HeapTag { tag: value }))
}

/// # Safety
/// `ptr` 必须来自 [`export_tag`] 且只 reclaim 一次。
pub unsafe fn reclaim_tag(ptr: *mut HeapTag) -> Box<HeapTag> {
    Box::from_raw(ptr)
}

/// Strict Provenance 违规（**仅文档**；本地可 `cargo +nightly miri test -- --ignored` 观察失败）。
#[cfg(test)]
mod strict_provenance_bad {
    //! ```ignore
    //! // 将整数地址当作指针解引用 —— Miri strict provenance 下为 UB
    //! let addr = 0x1234usize;
    //! let _ = unsafe { *(addr as *const u8) };
    //! ```

    #[test]
    #[ignore = "demonstrates Miri strict-provenance failure; run with --ignored"]
    fn int_to_ptr_is_ub() {
        let addr = 0x1234_usize;
        let _ = unsafe { *(addr as *const u8) };
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn swap_works() {
        let mut x = 1;
        let mut y = 2;
        swap_if_different(&mut x, &mut y);
        assert_eq!((x, y), (2, 1));
    }

    #[test]
    fn box_round_trip() {
        let ptr = export_tag(42);
        unsafe {
            assert_eq!((*ptr).tag, 42);
            let boxed = reclaim_tag(ptr);
            assert_eq!(boxed.tag, 42);
        }
    }
}
