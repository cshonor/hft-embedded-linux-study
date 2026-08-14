//! §10.2：标准库非阻塞 `TcpListener` + `WouldBlock` 轮询。
//!
//! 仅使用 `std`；`thread::sleep` 仅作教学轮询间隔，生产应换为 epoll/IOCP 等驱动。
use std::io::{self, Read, Write};
use std::net::TcpListener;
use std::thread;
use std::time::Duration;

fn main() -> io::Result<()> {
    let listener = TcpListener::bind("127.0.0.1:0")?;
    listener.set_nonblocking(true)?;
    let addr = listener.local_addr()?;
    println!("listening on {addr}");

    thread::spawn(move || {
        thread::sleep(Duration::from_millis(40));
        if let Ok(mut s) = std::net::TcpStream::connect(addr) {
            let _ = s.write_all(b"ping");
        }
    });

    for _ in 0..200 {
        match listener.accept() {
            Ok((mut stream, peer)) => {
                println!("accepted {peer}");
                let mut buf = [0u8; 16];
                match stream.read(&mut buf) {
                    Ok(n) => println!("read {n} bytes: {:?}", &buf[..n]),
                    Err(e) if e.kind() == io::ErrorKind::WouldBlock => {}
                    Err(e) => return Err(e),
                }
                break;
            }
            Err(e) if e.kind() == io::ErrorKind::WouldBlock => {
                thread::sleep(Duration::from_millis(10));
            }
            Err(e) => return Err(e),
        }
    }

    Ok(())
}
