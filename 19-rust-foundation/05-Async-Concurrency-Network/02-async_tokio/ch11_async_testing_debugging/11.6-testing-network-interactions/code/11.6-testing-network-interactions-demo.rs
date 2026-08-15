//! §11.6：最小本地 HTTP 回显（理解 mock 服务器；生产用 mockito/wiremock）。

use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::thread;
use std::time::Duration;

fn handle(mut stream: TcpStream) {
    let mut buf = [0u8; 512];
    let _ = stream.read(&mut buf);
    let body = r#"{"ok":true}"#;
    let resp = format!(
        "HTTP/1.1 201 Created\r\nContent-Length: {}\r\n\r\n{}",
        body.len(),
        body
    );
    let _ = stream.write_all(resp.as_bytes());
}

fn main() {
    let listener = TcpListener::bind("127.0.0.1:0").expect("bind");
    let addr = listener.local_addr().unwrap();
    thread::spawn(move || {
        if let Ok((s, _)) = listener.accept() {
            handle(s);
        }
    });
    thread::sleep(Duration::from_millis(50));
    let mut client = std::net::TcpStream::connect(addr).expect("connect");
    client
        .write_all(b"GET /items HTTP/1.1\r\nHost: localhost\r\n\r\n")
        .unwrap();
    let mut out = String::new();
    client.read_to_string(&mut out).unwrap();
    assert!(out.contains("201"), "mock server should return 201");
    println!("§11.6 ok — mockito 可替换为库内建 mock + expect(n)");
}
