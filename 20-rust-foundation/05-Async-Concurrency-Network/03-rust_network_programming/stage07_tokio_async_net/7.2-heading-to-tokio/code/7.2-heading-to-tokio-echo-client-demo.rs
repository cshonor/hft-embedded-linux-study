//! §7.2：异步客户端（需 `demo_7_2_echo_server`）。
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::TcpStream;

const ADDR: &str = "127.0.0.1:37082";

#[tokio::main]
async fn main() -> std::io::Result<()> {
    let mut stream = TcpStream::connect(ADDR).await?;
    stream.write_all(b"tokio 7.2\n").await?;
    let mut buf = [0u8; 1024];
    let n = stream.read(&mut buf).await?;
    println!("echo: {}", String::from_utf8_lossy(&buf[..n]));
    Ok(())
}
