//! §11.3：用 `timeout` 检测「过久未完成」的路径（教学；死锁探测思路）。
use std::time::Duration;

use tokio::time::{sleep, timeout};

#[tokio::main]
async fn main() {
    let work = async {
        sleep(Duration::from_secs(10)).await;
        "done"
    };

    match timeout(Duration::from_millis(250), work).await {
        Ok(v) => println!("完成: {v:?}"),
        Err(_) => println!("250ms 内未完成：在真实测试里可据此判定可疑卡死/过慢路径。"),
    }
}
