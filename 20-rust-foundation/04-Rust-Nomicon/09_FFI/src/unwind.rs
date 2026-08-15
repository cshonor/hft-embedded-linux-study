//! FFI 边界：`catch_unwind` 防止 panic 跨入 C。
//!
//! `extern "C" fn` 本身 **不可 unwind**（panic 即 abort）；须在入口内捕获。

use std::panic::{self, AssertUnwindSafe};

fn may_panic_rust(x: i32) -> i32 {
    if x < 0 {
        panic!("negative not allowed");
    }
    x + 1
}

/// Rust 侧 Safe 包装。
pub fn call_with_catch(x: i32) -> Result<i32, ()> {
    panic::catch_unwind(AssertUnwindSafe(|| may_panic_rust(x))).map_err(|_| ())
}

/// 导出给 C 的入口：**自身不 panic**，内部 `catch_unwind`。
#[no_mangle]
pub extern "C" fn safe_ffi_entry(x: i32) -> i32 {
    match call_with_catch(x) {
        Ok(v) => v,
        Err(()) => -1,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn ok_path() {
        assert_eq!(call_with_catch(1), Ok(2));
    }

    #[test]
    fn panic_caught() {
        assert_eq!(call_with_catch(-1), Err(()));
    }

    #[test]
    fn c_entry_maps_err() {
        assert_eq!(safe_ffi_entry(1), 2);
        assert_eq!(safe_ffi_entry(-1), -1);
    }
}
