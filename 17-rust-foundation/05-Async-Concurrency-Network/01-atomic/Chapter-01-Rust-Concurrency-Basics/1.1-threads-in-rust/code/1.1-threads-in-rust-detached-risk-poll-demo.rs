//! 分离态示例：主线程处理下单，后台线程定时拉行情做风控（教学 stub）。
//!
//! 运行：`rustc --crate-name detached_risk_poll_demo 1.1-threads-in-rust-detached-risk-poll-demo.rs && ./detached_risk_poll_demo`
//! 或在仓库根用 `cargo run --manifest-path ...`（若已接入 workspace 可改路径）。

use std::sync::{Arc, Mutex};
use std::thread;
use std::time::{Duration, SystemTime, UNIX_EPOCH};

#[derive(Debug, Default)]
struct RiskSnapshot {
    last_check_ms: u64,
    max_notional_ok: bool,
}

fn main() {
    let risk = Arc::new(Mutex::new(RiskSnapshot::default()));

    let risk_bg = Arc::clone(&risk);
    let handle = thread::spawn(move || {
        loop {
            let ok = poll_market_and_check_risk();
            {
                let mut snap = risk_bg.lock().unwrap();
                snap.last_check_ms = now_ms();
                snap.max_notional_ok = ok;
            }
            // 生产常见 10s；demo 用 2s 便于观察
            thread::sleep(Duration::from_secs(2));
        }
    });

    // 分离态：不 join、不阻塞主线程；后台一直跑到进程结束或 panic
    drop(handle);

    for i in 0..6 {
        let snap = risk.lock().unwrap();
        if snap.max_notional_ok {
            println!("[主线程] 下单 #{i}（最近风控 tick: {} ms）", snap.last_check_ms);
        } else {
            println!("[主线程] 下单 #{i} 暂缓（等待首次风控结果）");
        }
        drop(snap);
        thread::sleep(Duration::from_millis(400));
    }

    println!("[主线程] 核心交易循环结束；进程退出时 detached 风控线程一并终止");
}

fn poll_market_and_check_risk() -> bool {
    println!("[风控线程] 拉取全市场行情并校验…");
    true
}

fn now_ms() -> u64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap()
        .as_millis() as u64
}
