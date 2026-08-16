//! Safe 层：封装 `er-add-sys`，对外仅 safe API（Item 35 / Item 16）

/// 两整数相加（C 实现，Rust safe 封装）
pub fn add(a: i32, b: i32) -> i32 {
    unsafe { er_add_sys::er_add(a, b) }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn safe_add() {
        assert_eq!(add(40, 2), 42);
    }
}
