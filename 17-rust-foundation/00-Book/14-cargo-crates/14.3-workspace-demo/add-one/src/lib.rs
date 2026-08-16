//! 工作空间成员示例：提供 `add_one`。
//!
//! `Cargo.toml` 中的 `rand` 依赖会写入**工作空间根**的 `Cargo.lock`；本书用它在说明「多 crate 共享同一 lock 文件」。

pub fn add_one(x: i32) -> i32 {
    x + 1
}

#[cfg(test)]
mod tests {
    use super::*;
    use rand::Rng;

    #[test]
    fn it_works() {
        let _ = rand::thread_rng().gen::<u32>();
        assert_eq!(3, add_one(2));
    }
}
