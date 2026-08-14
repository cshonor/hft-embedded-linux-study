//! §7.4：捕获 Ctrl+C 后打印收尾（需 Tokio `signal` feature）。
use tokio::signal;

#[tokio::main]
async fn main() {
    println!("按 Ctrl+C 触发优雅停机演示…");
    signal::ctrl_c()
        .await
        .expect("listen ctrl_c");
    println!("收到退出信号，可在此处 await 未完成句柄、刷盘等。");
}
