//! P10 的 Rust 对照入口。
//!
//!   cargo test
//!   cargo run --release
//!   cargo run --release -- --jump 0
//!
//! 源码阅读顺序：types.rs → book.rs → strategy.rs → risk.rs → engine.rs → replay.rs

use hft_demo::replay::ReplayConfig;
use hft_demo::run_demo;

fn usage() {
    eprintln!("usage:");
    eprintln!("  hft_demo                 默认 2 万 tick 做市回放");
    eprintln!("  hft_demo [--ticks N] [--seed N] [--hits P] [--jump N]");
    eprintln!("  hft_demo --jump 0        关掉跳价，对比 PnL（逆向选择）");
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    if args.iter().any(|a| a == "-h" || a == "--help") {
        usage();
        return;
    }

    let mut cfg = ReplayConfig::default();
    let mut i = 1;
    while i < args.len() {
        match args[i].as_str() {
            "--ticks" => {
                cfg.ticks = next_val(&args, &mut i).parse().unwrap_or(0);
            }
            "--seed" => {
                cfg.seed = next_val(&args, &mut i).parse().unwrap_or(1);
            }
            "--hits" => {
                cfg.hit_prob = next_val(&args, &mut i).parse().unwrap_or(0.35);
            }
            "--jump" => {
                cfg.jump_every = next_val(&args, &mut i).parse().unwrap_or(0);
            }
            other => {
                eprintln!("unknown arg: {other}");
                usage();
                std::process::exit(2);
            }
        }
        i += 1;
    }

    if cfg.ticks <= 0 {
        eprintln!("ticks must be > 0");
        std::process::exit(2);
    }

    println!(
        "18-rust-quant demo  ticks={}  seed={}  hit_prob={:.2}  jump_every={}",
        cfg.ticks, cfg.seed, cfg.hit_prob, cfg.jump_every
    );
    println!("pipeline: replay --> book -> market-maker -> risk -> match -> PnL");
    println!("（单线程；C++ 对照是 P10 的 replay --SPSC--> engine）");

    run_demo(cfg, true);
}

fn next_val(args: &[String], i: &mut usize) -> String {
    if *i + 1 >= args.len() {
        usage();
        std::process::exit(2);
    }
    *i += 1;
    args[*i].clone()
}
