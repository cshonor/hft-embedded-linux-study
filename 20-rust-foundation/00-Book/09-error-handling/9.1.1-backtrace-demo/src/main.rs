// 9.1.1 Backtrace demo — 自动 panic 栈 · 手动 capture · catch + force_capture

use std::backtrace::Backtrace;
use std::panic::catch_unwind;

fn func_b() {
    let bt = Backtrace::capture();
    println!("=== 手动 Backtrace::capture() ===\n{bt}");
}

fn func_a() {
    func_b();
}

fn risky_oob() {
    let v = vec![1, 2, 3];
    let _ = v[10];
}

fn main() {
    let mode = std::env::args().nth(1).unwrap_or_else(|| "help".to_string());

    match mode.as_str() {
        "manual" => {
            func_a();
        }
        "oob" => {
            risky_oob();
        }
        "catch" => {
            let res = catch_unwind(risky_oob);
            if res.is_err() {
                println!(
                    "=== catch_unwind 后 Backtrace::force_capture() ===\n{}",
                    Backtrace::force_capture()
                );
            }
        }
        _ => {
            eprintln!(
                "usage:\n\
                  cargo run -- manual   # std::backtrace 手动打印\n\
                  cargo run -- oob      # 越界 panic（配 RUST_BACKTRACE=1）\n\
                  cargo run -- catch    # catch + force_capture\n\
                 \n\
                 PowerShell:\n\
                  $env:RUST_BACKTRACE=1\n\
                  cargo run -- oob\n\
                  $env:RUST_LIB_BACKTRACE=1\n\
                  cargo run -- manual"
            );
        }
    }
}
