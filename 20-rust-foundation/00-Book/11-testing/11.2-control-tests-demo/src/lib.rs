//! 11.2 控制测试如何运行 — 配套 demo
//!
//! ```bash
//! cd 00-Book/11-testing/11.2-control-tests-demo
//! cargo test                                    # 5 通过 + 3 ignored
//! cargo test show_print -- --show-output
//! cargo test add
//! cargo test -- --ignored                         # expensive_test（约 2s）
//! cargo test file -- --test-threads=1 --include-ignored  # 文件竞争串行演示
//! ```

pub fn add_two(a: i32) -> i32 {
    a + 2
}

#[cfg(test)]
mod tests {
    // ── §二 并行 vs 串行：共享 tmp.txt（默认 ignore，需串行）──
    #[test]
    #[ignore = "并行时与 test_file_b 竞争 tmp.txt；见 11.2 §二"]
    fn test_file_a() {
        std::fs::write("tmp.txt", "A").unwrap();
    }

    #[test]
    #[ignore = "须与 test_file_a 串行：cargo test file -- --test-threads=1 --include-ignored"]
    fn test_file_b() {
        let s = std::fs::read_to_string("tmp.txt").unwrap();
        println!("test_file_b read: {}", s);
        assert_eq!(s, "A");
        let _ = std::fs::remove_file("tmp.txt");
    }

    // ── §三 --show-output ───────────────────────────────
    #[test]
    fn show_print() {
        println!("我是测试内部打印内容");
        assert_eq!(1, 1);
    }

    // ── §四 名字过滤 ───────────────────────────────────
    #[test]
    fn add_two() {
        assert_eq!(super::add_two(2), 4);
    }

    #[test]
    fn add_two_and_two() {
        assert_eq!(2 + 2, 4);
    }

    #[test]
    fn one_hundred() {
        assert_eq!(100, 100);
    }

    // ── §五 #[ignore] 耗时 ──────────────────────────────
    #[test]
    #[ignore]
    fn expensive_test() {
        std::thread::sleep(std::time::Duration::from_secs(2));
        assert!(true);
    }
}
