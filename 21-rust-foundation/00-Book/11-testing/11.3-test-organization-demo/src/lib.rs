//! 11.3 测试组织结构 demo — 业务逻辑放 lib，便于单元 + 集成测试

fn private_add(a: i32, b: i32) -> i32 {
    a + b
}

pub fn pub_add(a: i32, b: i32) -> i32 {
    private_add(a, b)
}

pub fn calc(a: i32) -> i32 {
    a * 2
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_private() {
        assert_eq!(private_add(1, 2), 3);
    }

    #[test]
    fn test_pub() {
        assert_eq!(pub_add(2, 3), 5);
    }
}
