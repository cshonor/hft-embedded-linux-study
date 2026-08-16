//! §3.2：UDP 客户端（需先运行 `demo_3_2_udp_echo_server`）。
use std::net::UdpSocket;

const SERVER: &str = "127.0.0.1:39090";

fn main() -> std::io::Result<()> {
    let socket = UdpSocket::bind("127.0.0.1:0")?;
    let msg = b"hello udp 3.2";
    socket.send_to(msg, SERVER)?;
    let mut buf = [0u8; 1500];
    let (len, src) = socket.recv_from(&mut buf)?;
    println!("reply from {src}: {}", String::from_utf8_lossy(&buf[..len]));
    Ok(())
}
