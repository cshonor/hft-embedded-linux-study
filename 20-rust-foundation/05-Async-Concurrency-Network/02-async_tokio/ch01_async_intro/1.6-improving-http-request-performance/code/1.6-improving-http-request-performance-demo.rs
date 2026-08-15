//! 1.6 HTTP 性能：`tokio::join!` + `reqwest` 并发请求（需外网）。

use std::time::Instant;

use reqwest::Error;

#[tokio::main]
async fn main() -> Result<(), Error> {
    let url = "https://jsonplaceholder.typicode.com/posts/1";
    let start_time = Instant::now();

    let (_, _, _, _) = tokio::join!(
        reqwest::get(url),
        reqwest::get(url),
        reqwest::get(url),
        reqwest::get(url),
    );

    println!("并发请求总耗时: {} ms", start_time.elapsed().as_millis());
    Ok(())
}
