//! §9.1：async task vs OS thread（概念对比）。
use std::thread;
use std::time::Duration;

async fn async_work() {
    println!("async: state machine on Tokio executor");
}

fn main() {
    thread::spawn(|| {
        thread::sleep(Duration::from_millis(50));
        println!("thread: OS-scheduled stack");
    });
    let rt = tokio::runtime::Runtime::new().unwrap();
    rt.block_on(async_work());
}
