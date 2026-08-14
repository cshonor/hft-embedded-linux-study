use std::io::Read;

pub fn sum(a: i32, b: i32) -> i32 {
    a + b
}

/// 可测试：入参任意 `Read`（stdin / Cursor 内存流）
pub fn read_data<R: Read>(mut source: R) -> String {
    let mut buf = String::new();
    source.read_to_string(&mut buf).unwrap();
    buf.trim().to_string()
}

pub trait ConfigSource {
    fn get_config(&self) -> String;
}

pub fn load_app_config(source: impl ConfigSource) -> String {
    source.get_config()
}

#[cfg(not(test))]
pub fn init_log() {
    println!("init_log: 仅正式 build 编译此函数");
}

#[cfg(test)]
mod tests {
    use super::*;
    use mockall::mock;
    use std::io::Cursor;

    #[test]
    fn test_sum_1() {
        assert_eq!(sum(2, 3), 5);
    }

    #[test]
    fn test_sum_2() {
        assert_eq!(sum(-1, 1), 0);
    }

    #[test]
    fn mock_input_test() {
        let mock_input = Cursor::new("abc123");
        let result = read_data(mock_input);
        assert_eq!(result, "abc123");
    }

    mock! {
        FakeConfig {}
        impl ConfigSource for FakeConfig {
            fn get_config(&self) -> String;
        }
    }

    #[test]
    fn test_config_mock() {
        let mut mock = MockFakeConfig::new();
        mock.expect_get_config()
            .return_const("mock_config_data".to_string());
        assert_eq!(load_app_config(mock), "mock_config_data");
    }

    #[test]
    #[ignore]
    fn slow_placeholder() {}
}
