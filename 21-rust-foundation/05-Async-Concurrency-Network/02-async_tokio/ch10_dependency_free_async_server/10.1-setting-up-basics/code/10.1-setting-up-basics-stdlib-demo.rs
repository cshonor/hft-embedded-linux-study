// 本章配套示例：在本目录执行 `rustc demo.rs`，再运行生成的可执行文件。
// 需要 Tokio / 异步入口时，可把逻辑迁到主仓库 `src/async_tokio/` 下用 `cargo run`。

#![crate_name = "demo_10_1_basics"]
fn main() {
    println!("§10.1 — 完整 workspace 按书搭建 client/server/async_runtime");
}
