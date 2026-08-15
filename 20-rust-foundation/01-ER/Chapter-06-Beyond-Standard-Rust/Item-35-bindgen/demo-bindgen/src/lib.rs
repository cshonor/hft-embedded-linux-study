//! Item 35: build.rs + bindgen 从 C 头文件生成绑定

#![allow(
    non_upper_case_globals,
    non_camel_case_types,
    non_snake_case,
    dead_code
)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

/// Safe 封装：unsafe FFI 在内
pub fn add(a: i32, b: i32) -> i32 {
    unsafe { er_add(a, b) }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn add_via_bindgen() {
        assert_eq!(add(2, 40), 42);
    }
}
