//! §6.1：最小 HTTP/1.0 响应（`std::net`，理解 Hyper 之下的报文形态）。
use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};

fn handle(mut stream: TcpStream) -> std::io::Result<()> {
    let mut buf = [0u8; 1024];
    let _ = stream.read(&mut buf)?;
    let body = "Hello from 6.1 raw TCP HTTP demo (study_network_ch06)\n";
    let resp = format!(
        "HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\nContent-Length: {}\r\n\r\n{}",
        body.len(),
        body
    );
    stream.write_all(resp.as_bytes())?;
    stream.flush()?;
    Ok(())
}

fn main() -> std::io::Result<()> {
    let addr = "127.0.0.1:18080";
    let listener = TcpListener::bind(addr)?;
    println!("6.1 demo listening: http://{addr}/");
    println!("Try: curl http://{addr}/");
    for stream in listener.incoming() {
        if let Ok(s) = stream {
            let _ = handle(s);
        }
    }
    Ok(())
}
