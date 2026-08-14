//! ZST 辅助：用整数伪装指针推进，从 dangling 读取。

use std::mem;
use std::ptr::NonNull;

pub fn is_zst<T>() -> bool {
    mem::size_of::<T>() == 0
}

pub fn zst_read<T>() -> T {
    debug_assert!(is_zst::<T>());
    unsafe { std::ptr::read(NonNull::dangling().as_ptr()) }
}

/// ZST「指针」推进：转为 usize 计数再 cast 回指针。
pub fn zst_ptr_add<T>(ptr: *const T, delta: usize) -> *const T {
    debug_assert!(is_zst::<T>());
    (ptr as usize).wrapping_add(delta) as *const T
}

pub fn zst_cap() -> usize {
    usize::MAX
}

#[cfg(test)]
mod tests {
    use super::*;
    struct Nothing;

    #[test]
    fn zst_read_works() {
        let _: Nothing = zst_read();
    }
}
