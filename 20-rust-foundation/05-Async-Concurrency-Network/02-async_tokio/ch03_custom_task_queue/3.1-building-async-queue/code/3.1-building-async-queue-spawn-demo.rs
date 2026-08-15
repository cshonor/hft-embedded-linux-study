//! 3.1 Building Async Queue：`flume` 分发 Runnable 的最小示意（完整运行时见书 + async-task）。

use flume::{Receiver, Sender};

fn main() {
    let (tx, rx): (Sender<u32>, Receiver<u32>) = flume::unbounded();
    std::thread::spawn(move || {
        for id in 0..3 {
            tx.send(id).expect("enqueue task id");
        }
    });
    while let Ok(id) = rx.recv() {
        println!("worker polled task {id}");
    }
    println!("=== 3.1 minimal queue drain done ===");
}
