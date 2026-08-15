// 9.1 panic! demo — 主动/被动 panic · catch_unwind

use std::panic;

fn main() {
    let mode = std::env::args().nth(1).unwrap_or_else(|| "help".to_string());

    match mode.as_str() {
        "panic" => {
            // §二 主动 panic
            panic!("crash and burn");
        }
        "oob" => {
            // §二 被动：Vec 越界
            let v = vec![1, 2, 3];
            let _ = v[99];
        }
        "unwrap" => {
            // §二 被动：None.unwrap()
            let opt: Option<i32> = None;
            let _ = opt.unwrap();
        }
        "catch" => {
            // §一 catch_unwind（仅 panic=unwind 时有效；abort 模式进程直接退出）
            let result = panic::catch_unwind(|| {
                panic!("被 catch 的 panic");
            });
            match result {
                Ok(_) => println!("未 panic"),
                Err(_) => println!("catch_unwind 捕获成功（unwind 模式）"),
            }
        }
        _ => {
            eprintln!(
                "usage:\n\
                  cargo run -- panic     # 主动 panic!\n\
                  cargo run -- oob       # Vec 越界\n\
                  cargo run -- unwrap    # None.unwrap()\n\
                  cargo run -- catch     # catch_unwind\n\
                 \n\
                 Backtrace (PowerShell):\n\
                  $env:RUST_BACKTRACE=1\n\
                  cargo run -- oob"
            );
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn catch_unwind_works_in_test() {
        let caught = panic::catch_unwind(|| panic!("test panic"));
        assert!(caught.is_err());
    }
}
