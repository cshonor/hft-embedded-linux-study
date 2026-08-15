//! 4.9 Sending Data：先起 listener 线程，客户端连同一端口并 write。

use std::io::{Read, Write};
use std::net::TcpListener as StdListener;
use std::sync::mpsc;
use std::thread;
use std::time::Duration;

fn main() -> std::io::Result<()> {
    let (tx, rx) = mpsc::channel();
    thread::spawn(move || {
        let std = StdListener::bind("127.0.0.1:0").unwrap();
        let port = std.local_addr().unwrap().port();
        tx.send(port).unwrap();
        let (mut s, _) = std.accept().unwrap();
        let mut buf = [0u8; 16];
        let n = s.read(&mut buf).unwrap();
        println!("server got: {:?}", std::str::from_utf8(&buf[..n]));
    });

    thread::sleep(Duration::from_millis(50));
    let port = rx.recv().unwrap();
    let mut client = std::net::TcpStream::connect(("127.0.0.1", port))?;
    client.write_all(b"hello mio path")?;
    println!("=== 4.9 client sent over TCP ===");
    Ok(())
}
