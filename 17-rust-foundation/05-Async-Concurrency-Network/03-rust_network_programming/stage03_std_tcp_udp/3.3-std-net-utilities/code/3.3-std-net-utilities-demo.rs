//! §3.3：`SocketAddr` 解析与 `IpAddr` 属性。
use std::net::{IpAddr, SocketAddr};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let loopback: SocketAddr = "127.0.0.1:8080".parse()?;
    let dns: SocketAddr = "8.8.8.8:53".parse()?;
    println!("127.0.0.1:8080 loopback? {}", loopback.ip().is_loopback());
    println!("8.8.8.8:53 is_ipv4? {}", dns.ip().is_ipv4());
    let ip: IpAddr = "::1".parse()?;
    println!("::1 is_ipv6? {}", ip.is_ipv6());
    Ok(())
}
