// 本章配套示例：在本目录执行 `rustc demo.rs`，再运行生成的可执行文件。
// 需要 Tokio / 异步入口时，可把逻辑迁到主仓库 `src/async_tokio/` 下用 `cargo run`。

#![crate_name = "demo_11_1_sync_test"]
fn main() {
    println!("§11.1 — mockall 示例见 tests/ch11_database_mock.rs");
}
