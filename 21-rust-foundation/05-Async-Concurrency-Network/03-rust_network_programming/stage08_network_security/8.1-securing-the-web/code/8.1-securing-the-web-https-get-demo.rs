//! §8.1：HTTPS GET（rustls，需外网）。
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let client = reqwest::blocking::Client::new();
    let resp = client.get("https://httpbin.org/get").send()?;
    println!("HTTPS status: {}", resp.status());
    Ok(())
}
