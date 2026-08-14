//! Nomicon ch10：`#![no_std]` 库 + `#[panic_handler]`。
//!
//! 默认 feature `std` 便于宿主测试；`--no-default-features` 构建纯 no_std。

#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(not(feature = "std"))]
use core::panic::PanicInfo;

/// 可在 no_std 环境使用的纯 core API。
pub fn add(a: u32, b: u32) -> u32 {
    a.wrapping_add(b)
}

/// 简单字节求和（演示 `core` 切片）。
pub fn checksum(data: &[u8]) -> u32 {
    data.iter().fold(0u32, |acc, &b| acc.wrapping_add(u32::from(b)))
}

#[cfg(not(feature = "std"))]
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    // release 嵌入式常用 panic-halt：死循环挂起。
    loop {}
}

#[cfg(all(test, feature = "std"))]
mod tests {
    use super::*;

    #[test]
    fn add_works() {
        assert_eq!(add(40, 2), 42);
    }

    #[test]
    fn checksum_works() {
        assert_eq!(checksum(b"abc"), 294);
    }
}
