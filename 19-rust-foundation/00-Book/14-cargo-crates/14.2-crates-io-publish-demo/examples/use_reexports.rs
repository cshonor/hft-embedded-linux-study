//! 演示通过 [`pub use`] 重导出后，可从 crate 根直接引入类型与函数。
//!
//! 运行：`cargo run --example use_reexports`

use crates_io_publish_demo::{mix, PrimaryColor};

fn main() {
    let _ = mix(PrimaryColor::Red, PrimaryColor::Yellow);
}
