//! §11.4：多线程 Tokio 下 stress 计数（演示应用层竞态风险，非 UB 示范）。

use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::Arc;

#[tokio::main(flavor = "multi_thread", worker_threads = 4)]
async fn main() {
    let n = 200usize;
  // 安全版本：原子
    let ok = Arc::new(AtomicUsize::new(0));
    let mut handles = vec![];
    for _ in 0..n {
        let ok = Arc::clone(&ok);
        handles.push(tokio::spawn(async move {
            for _ in 0..100 {
                ok.fetch_add(1, Ordering::Relaxed);
            }
        }));
    }
    for h in handles {
        h.await.unwrap();
    }
    let got = ok.load(Ordering::Relaxed);
    let expect = n * 100;
    assert_eq!(got, expect, "atomic counter should be exact");
    println!("§11.4 ok: atomic count = {got} (expected {expect})");
}
