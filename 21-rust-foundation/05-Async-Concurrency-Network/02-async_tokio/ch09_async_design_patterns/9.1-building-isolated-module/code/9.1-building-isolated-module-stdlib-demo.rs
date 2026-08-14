// 本章配套示例：在本目录执行 `rustc demo.rs`，再运行生成的可执行文件。
// 需要 Tokio / 异步入口时，可把逻辑迁到主仓库 `src/async_tokio/` 下用 `cargo run`。

#![crate_name = "demo_9_1_isolated"]
fn main() {
    println!("§9.1 — 完整隔离模块 Facade 按书在 workspace 实现");
}
