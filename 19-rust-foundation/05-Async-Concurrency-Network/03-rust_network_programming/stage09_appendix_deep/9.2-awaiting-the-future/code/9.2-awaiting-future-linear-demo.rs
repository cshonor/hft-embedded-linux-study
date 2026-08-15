//! §9.2：线性 async/await（替代 futures-await 宏链）。
use tokio::time::{sleep, Duration};

async fn step1() -> u32 {
    sleep(Duration::from_millis(50)).await;
    1
}

async fn step2(v: u32) -> u32 {
    sleep(Duration::from_millis(50)).await;
    v + 2
}

#[tokio::main]
async fn main() {
    let result = step2(step1().await).await;
    println!("linear async result: {result}");
}
