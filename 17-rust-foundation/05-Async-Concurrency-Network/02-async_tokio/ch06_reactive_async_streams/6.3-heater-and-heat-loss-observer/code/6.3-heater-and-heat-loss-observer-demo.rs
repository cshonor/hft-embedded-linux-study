//! §6.3：加热器与散热观察者——按时间片修改 `TEMP`（注意延迟叠加可能导致振荡）。
use std::sync::atomic::{AtomicI16, Ordering};
use std::sync::Arc;
use std::time::Instant;

use tokio::time::{sleep, Duration};

static DESIRED_TEMP: AtomicI16 = AtomicI16::new(22);

#[tokio::main]
async fn main() {
    let temp = Arc::new(AtomicI16::new(18));
    let started = Instant::now();

    let heater = {
        let temp = Arc::clone(&temp);
        tokio::spawn(async move {
            loop {
                let elapsed = started.elapsed();
                if elapsed < Duration::from_millis(500) {
                    sleep(Duration::from_millis(50)).await;
                    continue;
                }
                let cur = temp.load(Ordering::Relaxed);
                let goal = DESIRED_TEMP.load(Ordering::Relaxed);
                if cur < goal {
                    temp.fetch_add(1, Ordering::Relaxed);
                    println!("[Heater] +1 -> {}", temp.load(Ordering::Relaxed));
                }
                if elapsed > Duration::from_secs(8) {
                    break;
                }
                sleep(Duration::from_millis(200)).await;
            }
        })
    };

    let heat_loss = {
        let temp = Arc::clone(&temp);
        tokio::spawn(async move {
            loop {
                let elapsed = started.elapsed();
                if elapsed < Duration::from_millis(800) {
                    sleep(Duration::from_millis(50)).await;
                    continue;
                }
                let cur = temp.load(Ordering::Relaxed);
                if cur > 16 {
                    temp.fetch_sub(1, Ordering::Relaxed);
                    println!("[HeatLoss] -1 -> {}", temp.load(Ordering::Relaxed));
                }
                if elapsed > Duration::from_secs(8) {
                    break;
                }
                sleep(Duration::from_millis(250)).await;
            }
        })
    };

    let _ = tokio::join!(heater, heat_loss);
    println!(
        "Final temp={} (oscillation risk when heater/loss delays differ)",
        temp.load(Ordering::Relaxed)
    );
}
