//! 5.3 Mimicking Async：单线程 Executor 轮询 3 个「休眠」协程，墙钟约 1s。

use std::thread;
use std::time::{Duration, Instant};

enum CoStep {
    Running,
    Done,
}

struct SleepCo {
    until: Instant,
}

impl SleepCo {
    fn new(ms: u64) -> Self {
        Self {
            until: Instant::now() + Duration::from_millis(ms),
        }
    }

    fn resume(&mut self) -> CoStep {
        if Instant::now() >= self.until {
            CoStep::Done
        } else {
            CoStep::Running
        }
    }
}

fn main() {
    let start = Instant::now();
    let mut cos = [SleepCo::new(1000), SleepCo::new(1000), SleepCo::new(1000)];
    let mut finished = [false; 3];
    while finished.iter().any(|&f| !f) {
        for (i, c) in cos.iter_mut().enumerate() {
            if !finished[i] && matches!(c.resume(), CoStep::Done) {
                finished[i] = true;
            }
        }
        thread::sleep(Duration::from_millis(10));
    }
    println!(
        "=== 5.3 three 1s coroutines finished in {:?} (cooperative poll) ===",
        start.elapsed()
    );
}
