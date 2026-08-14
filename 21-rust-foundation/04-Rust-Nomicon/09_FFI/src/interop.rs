//! `repr(C)` 结构体与 `CString`。

use std::ffi::{CStr, CString};
use std::os::raw::c_int;

#[repr(C)]
pub struct Point {
    pub x: c_int,
    pub y: c_int,
}

pub fn make_point(x: i32, y: i32) -> Point {
    Point { x, y }
}

pub fn rust_string_to_c(s: &str) -> CString {
    CString::new(s).expect("no interior nul")
}

pub fn c_string_to_rust(c: &CStr) -> String {
    c.to_string_lossy().into_owned()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn cstring_roundtrip() {
        let c = rust_string_to_c("ffi");
        assert_eq!(c_string_to_rust(c.as_c_str()), "ffi");
    }

    #[test]
    fn repr_c_layout() {
        let p = make_point(1, 2);
        assert_eq!(p.x, 1);
    }
}
