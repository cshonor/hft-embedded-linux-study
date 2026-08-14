// 9.1.3 catch_unwind vs panic=unwind/abort

use std::panic;
use std::thread;

fn inner_with_drop() {
    let _s = String::from("临时资源"); // Drop 在 unwind 时执行
    panic!("inner 出错了");
}

fn run_catch_demo() {
    println!("--- catch_unwind(inner_with_drop) ---");
    let ret = panic::catch_unwind(inner_with_drop);
    match ret {
        Ok(_) => println!("未 panic"),
        Err(_) => println!("✅ 捕获 panic，main 继续运行（须 panic=unwind）"),
    }
    println!("main 正常结束");
}

fn run_thread_demo() {
    println!("--- 子线程 panic（unwind 下主线程通常继续）---");
    let handle = thread::spawn(|| {
        panic!("子线程 panic");
    });
    let _ = handle.join();
    println!("主线程仍在运行（join 返回 Err）");
}

fn main() {
    let mode = std::env::args().nth(1).unwrap_or_else(|| "help".to_string());

    match mode.as_str() {
        "catch" => run_catch_demo(),
        "thread" => run_thread_demo(),
        _ => {
            eprintln!(
                "usage:\n\
                  cargo run -- catch          # dev/unwind → 捕获成功\n\
                  cargo run --release -- catch # release/abort → 进程退出\n\
                  cargo run --profile release-unwind -- catch\n\
                  cargo run -- thread"
            );
        }
    }
}

#[cfg(test)]
mod tests {
    use std::panic;

    #[test]
    fn catch_unwind_in_test() {
        let r = panic::catch_unwind(|| panic!("x"));
        assert!(r.is_err());
    }
}
