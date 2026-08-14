//! §6.2：Display 观察者——`SeqCst` 快照 + `compare_exchange` 控加热器开关。
use std::sync::atomic::{AtomicBool, AtomicI16, Ordering};

use tokio::time::{sleep, Duration};

static TEMP: AtomicI16 = AtomicI16::new(18);
static DESIRED_TEMP: AtomicI16 = AtomicI16::new(22);
static HEATER_ON: AtomicBool = AtomicBool::new(false);

#[tokio::main]
async fn main() {
    // 模拟外部任务缓慢升温（供显示器观察）
    tokio::spawn(async {
        for _ in 0..20 {
            TEMP.fetch_add(1, Ordering::Relaxed);
            sleep(Duration::from_millis(120)).await;
        }
    });

    let mut last = i16::MIN;
    for _ in 0..40 {
        let cur = TEMP.load(Ordering::SeqCst);
        let goal = DESIRED_TEMP.load(Ordering::SeqCst);
        let want_on = cur < goal;

        // 仅当开关仍为「与目标相反」的期望值时才翻转，避免盲目 store
        let _ = HEATER_ON.compare_exchange(
            !want_on,
            want_on,
            Ordering::SeqCst,
            Ordering::SeqCst,
        );

        if cur != last {
            println!(
                "[Display] temp={cur}°C goal={goal}°C heater={}",
                HEATER_ON.load(Ordering::SeqCst)
            );
            last = cur;
        }
        sleep(Duration::from_millis(60)).await;
    }
}
