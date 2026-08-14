//! §3.1：TCP 回显客户端（需先运行 `demo_3_1_tcp_echo_server`）。
use std::io::{Read, Write};
use std::net::TcpStream;

const ADDR: &str = "127.0.0.1:38081";

fn main() -> std::io::Result<()> {
    let mut stream = TcpStream::connect(ADDR)?;
    let msg = b"hello from 3.1 tcp client\n";
    stream.write_all(msg)?;
    let mut buf = [0u8; 1024];
    let n = stream.read(&mut buf)?;
    println!("sent: {}", String::from_utf8_lossy(msg));
    println!("echo: {}", String::from_utf8_lossy(&buf[..n]));
    Ok(())
}
