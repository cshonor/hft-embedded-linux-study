//! RFR ch03 · 01-1 ~ 01-5 命名五系列 demo
//!
//! ```bash
//! cargo run --manifest-path naming-series-demo/Cargo.toml          # 全部
//! cargo run --manifest-path naming-series-demo/Cargo.toml as         # 01-1
//! cargo run --manifest-path naming-series-demo/Cargo.toml into       # 01-2
//! cargo run --manifest-path naming-series-demo/Cargo.toml get        # 01-3
//! cargo run --manifest-path naming-series-demo/Cargo.toml try        # 01-4
//! cargo run --manifest-path naming-series-demo/Cargo.toml with       # 01-5
//! cargo run --manifest-path naming-series-demo/Cargo.toml into-inner # 01-2-1
//! ```

mod as_series;
mod get_series;
mod into_inner_series;
mod into_series;
mod try_series;
mod with_series;

use std::env;

fn main() {
    match env::args().nth(1).as_deref() {
        Some("as") => as_series::run(),
        Some("into") => into_series::run(),
        Some("into-inner") => into_inner_series::run(),
        Some("get") => get_series::run(),
        Some("try") => try_series::run(),
        Some("with") => with_series::run(),
        None => run_all(),
        Some(other) => panic!("unknown subcommand: {other}"),
    }
}

fn run_all() {
    as_series::run();
    into_series::run();
    into_inner_series::run();
    get_series::run();
    try_series::run();
    with_series::run();
    println!("all naming-series demos ok");
}
