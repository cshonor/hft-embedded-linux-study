//! §9.5：重试 + 指数退避。
//!
//! 书中常用 `Duration::from_secs(1 << i)`；此处改为毫秒级，避免本地演示等待过久。
use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::Arc;
use std::time::Duration;

use tokio::time::sleep;

/// 模拟：前 `failures` 次返回 Err，之后成功。
async fn perform_async_call(failures: &Arc<AtomicU32>) -> Result<(), String> {
    let n = failures.fetch_add(1, Ordering::Relaxed);
    if n < 2 {
        Err(format!("simulated error (attempt {n})"))
    } else {
        Ok(())
    }
}

async fn do_something(failures: Arc<AtomicU32>) -> Result<(), String> {
    for i in 0..5u32 {
        match perform_async_call(&failures).await {
            Ok(()) => return Ok(()),
            Err(e) => {
                println!("发生错误: {e}，正在重试…");
                let ms = 20u64 * (1u64 << i.min(6));
                sleep(Duration::from_millis(ms)).await;
            }
        }
    }
    Err("重试次数耗尽".to_string())
}

#[tokio::main]
async fn main() {
    let failures = Arc::new(AtomicU32::new(0));
    match do_something(failures).await {
        Ok(()) => println!("成功"),
        Err(e) => println!("{e}"),
    }
}
