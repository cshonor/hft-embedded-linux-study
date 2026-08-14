//! §7.1：`Builder::new_multi_thread` 线程钩子 + `max_blocking_threads`（教学向最小示例）。
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::Arc;

use tokio::runtime::Builder;

fn main() {
    let starts = Arc::new(AtomicUsize::new(0));
    let stops = Arc::new(AtomicUsize::new(0));

    let starts_cb = Arc::clone(&starts);
    let stops_cb = Arc::clone(&stops);

    let rt = Builder::new_multi_thread()
        .thread_name_fn({
            let idx = Arc::new(AtomicUsize::new(0));
            move || {
                let n = idx.fetch_add(1, Ordering::Relaxed);
                format!("demo-worker-{n}")
            }
        })
        .on_thread_start(move || {
            starts_cb.fetch_add(1, Ordering::Relaxed);
        })
        .on_thread_stop(move || {
            stops_cb.fetch_add(1, Ordering::Relaxed);
        })
        .max_blocking_threads(2)
        .enable_all()
        .build()
        .expect("build runtime");

    rt.block_on(async {
        tokio::time::sleep(std::time::Duration::from_millis(20)).await;
    });

    drop(rt);

    println!(
        "on_thread_start 调用约 {} 次，on_thread_stop 约 {} 次（含阻塞池线程，具体数以运行时策略为准）",
        starts.load(Ordering::Relaxed),
        stops.load(Ordering::Relaxed),
    );
}
