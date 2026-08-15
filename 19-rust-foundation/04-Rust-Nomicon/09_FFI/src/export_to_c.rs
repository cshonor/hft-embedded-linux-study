//! 从 C 调用 Rust：`extern "C"` + `#[no_mangle]`。

/// C 侧声明：`int32_t rust_add(int32_t a, int32_t b);`
#[no_mangle]
pub extern "C" fn rust_add(a: i32, b: i32) -> i32 {
    a + b
}

#[no_mangle]
pub extern "C" fn rust_greet() -> *const std::os::raw::c_char {
    // 静态 C 字符串；生产代码注意生命周期与并发。
    b"hello from rust\0".as_ptr() as *const std::os::raw::c_char
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn add_via_c_abi() {
        assert_eq!(rust_add(2, 5), 7);
    }
}
