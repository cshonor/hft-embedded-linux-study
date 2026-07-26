//! UNP 1.2 · Daytime TCP 客户端（Rust，逻辑对齐 C 版 daytimetcpcli.c）
//!
//! 用法：
//!   rustc daytimetcpcli.rs && ./daytimetcpcli 127.0.0.1
//! 或在本目录：
//!   cargo run -- 127.0.0.1
//!
//! 要点：TCP 字节流须循环 read；read 返回 0 表示对端 FIN。

use std::env;
use std::io::{self, Read};
use std::net::TcpStream;
use std::process;

/// 与 UNP `MAXLINE` 一致
const MAXLINE: usize = 4096;

fn main() {
    if let Err(e) = run() {
        eprintln!("{}", e);
        process::exit(1);
    }
}

fn run() -> io::Result<()> {
    // 等价 C：argc != 2
    let args: Vec<String> = env::args().collect();
    if args.len() != 2 {
        eprintln!("usage: {} <IPaddress>", args[0]);
        process::exit(1);
    }
    let host = &args[1];

    // 等价：sockaddr_in + htons(13) + inet_pton → "IP:13"
    // 标准库在 connect 时做地址解析与网络字节序，无需手写 htons
    // IPv6 请写 [::1]:13 这种形式
    let addr = format!("{host}:13");

    // 等价：socket + connect（内部完成三次握手）
    // 失败则不能复用：须重新 TcpStream::connect（同 C 不能对同一 fd 再 connect）
    let mut stream = TcpStream::connect(&addr).map_err(|e| {
        io::Error::new(
            e.kind(),
            format!("connect error ({addr}): {e}"),
        )
    })?;

    let mut buf = [0u8; MAXLINE];

    // 等价：while ((n = read(...)) > 0) { recvline[n]=0; fputs(...); }
    loop {
        let n = stream.read(&mut buf)?;
        if n == 0 {
            // 对端关闭，收到 FIN，与 C read==0 一致
            break;
        }
        // Rust 用字节切片输出，不必手动补 '\0'
        print!("{}", String::from_utf8_lossy(&buf[..n]));
    }

    // stream 离开作用域 Drop → close fd → 发 FIN（等价 exit 回收 fd）
    Ok(())
}
