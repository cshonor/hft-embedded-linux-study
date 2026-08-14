//! §3.1：多线程 TCP 回显服务端（`127.0.0.1:38081`）。
use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::thread;

const ADDR: &str = "127.0.0.1:38081";

fn handle_client(mut stream: TcpStream) -> std::io::Result<()> {
    let mut buf = [0u8; 1024];
    let n = stream.read(&mut buf)?;
    stream.write_all(&buf[..n])?;
    stream.flush()?;
    Ok(())
}

fn main() -> std::io::Result<()> {
    let listener = TcpListener::bind(ADDR)?;
    println!("3.1 TCP echo server on {ADDR} (Ctrl+C to stop)");
    for stream in listener.incoming() {
        let stream = stream?;
        thread::spawn(move || {
            if let Err(e) = handle_client(stream) {
                eprintln!("client error: {e}");
            }
        });
    }
    Ok(())
}
