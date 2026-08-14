//! §6.3：`reqwest` 阻塞 GET（需可访问外网）。

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let url = "https://httpbin.org/get";
    let client = reqwest::blocking::Client::new();
    let resp = client.get(url).send()?;
    println!("status: {}", resp.status());
    let body = resp.text()?;
    let preview: String = body.chars().take(200).collect();
    println!("body (first 200 chars): {preview}...");
    Ok(())
}
