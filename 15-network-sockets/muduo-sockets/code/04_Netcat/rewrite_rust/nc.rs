//! PNP 04 · 最小 Netcat（Rust，逻辑对齐课程 nc 实验）
//!
//! 用法：
//!   cargo run -- 127.0.0.1 9000    # 客户端：stdin ↔ TCP
//!   cargo run -- -l 9000           # 服务端：监听，接受一个连接后双向转发
//!
//! 要点：TCP 字节流管道；一端 close 写 → 对端 read 得 0（半关闭语义由 OS 传递）。

use std::env;
use std::io::{self, copy, stdin, stdout};
use std::net::{TcpListener, TcpStream};
use std::process;
use std::thread;

fn main() {
    if let Err(e) = run() {
        eprintln!("{}", e);
        process::exit(1);
    }
}

fn run() -> io::Result<()> {
    let args: Vec<String> = env::args().collect();
    match args.len() {
        3 if args[1] == "-l" => listen_and_relay(&args[2]),
        3 => connect_and_relay(&args[1], &args[2]),
        _ => usage(&args[0]),
    }
}

fn usage(prog: &str) -> ! {
    eprintln!("usage:");
    eprintln!("  {prog} <host> <port>   # connect");
    eprintln!("  {prog} -l <port>        # listen (single client)");
    process::exit(1);
}

fn listen_and_relay(port: &str) -> io::Result<()> {
    let addr = format!("0.0.0.0:{port}");
    let listener = TcpListener::bind(&addr)?;
    eprintln!("listening on {addr}");
    let (stream, peer) = listener.accept()?;
    eprintln!("connected from {peer}");
    relay(stream)
}

fn connect_and_relay(host: &str, port: &str) -> io::Result<()> {
    let stream = TcpStream::connect(format!("{host}:{port}"))?;
    relay(stream)
}

/// 双向拷贝：socket → stdout（子线程），stdin → socket（主线程）
fn relay(stream: TcpStream) -> io::Result<()> {
    let mut from_sock = stream.try_clone()?;
    let mut to_sock = stream;

    let reader = thread::spawn(move || {
        let _ = copy(&mut from_sock, &mut stdout());
    });

    let _ = copy(&mut stdin(), &mut to_sock)?;
    let _ = reader.join();
    Ok(())
}
