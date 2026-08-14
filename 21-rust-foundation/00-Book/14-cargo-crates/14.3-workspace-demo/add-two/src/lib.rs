//! 工作空间成员示例：提供 `add_two`。

pub fn add_two(x: i32) -> i32 {
    x + 2
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn two_more() {
        assert_eq!(5, add_two(3));
    }
}
