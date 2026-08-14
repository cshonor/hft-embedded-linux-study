//! §7.2：Tokio 异步 TCP 回显（`127.0.0.1:37082`）。
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::TcpListener;

const ADDR: &str = "127.0.0.1:37082";

#[tokio::main]
async fn main() -> std::io::Result<()> {
    let listener = TcpListener::bind(ADDR).await?;
    println!("7.2 async echo on {ADDR}");
    loop {
        let (mut socket, addr) = listener.accept().await?;
        tokio::spawn(async move {
            let mut buf = [0u8; 1024];
            match socket.read(&mut buf).await {
                Ok(0) => return,
                Ok(n) => {
                    let _ = socket.write_all(&buf[..n]).await;
                    println!("echoed {n} bytes for {addr}");
                }
                Err(e) => eprintln!("read error: {e}"),
            }
        });
    }
}
