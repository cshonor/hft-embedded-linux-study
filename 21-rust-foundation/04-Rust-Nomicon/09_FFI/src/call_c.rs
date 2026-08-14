//! 从 Rust 调用 C：extern 块 + unsafe + Safe 包装。

extern "C" {
    fn abs(input: i32) -> i32;
}

/// Safe 包装：假定 C `abs` 满足其文档契约。
pub fn abs_i32_safe(n: i32) -> i32 {
    unsafe { abs(n) }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn abs_works() {
        assert_eq!(abs_i32_safe(-7), 7);
        assert_eq!(abs_i32_safe(3), 3);
    }
}
