//! §3.2：UDP 回显服务端（`127.0.0.1:39090`）。
use std::net::UdpSocket;

const ADDR: &str = "127.0.0.1:39090";

fn main() -> std::io::Result<()> {
    let socket = UdpSocket::bind(ADDR)?;
    println!("3.2 UDP echo server on {ADDR}");
    let mut buf = [0u8; 1500];
    loop {
        let (len, src) = socket.recv_from(&mut buf)?;
        socket.send_to(&buf[..len], src)?;
        println!("echoed {len} bytes to {src}");
    }
}
