//! §3.2.1：向多播组发送一条报文（需 `demo_3_2_1_multicast_recv` 在运行）。
use std::net::{Ipv4Addr, SocketAddrV4, UdpSocket};

fn main() -> std::io::Result<()> {
    let socket = UdpSocket::bind("0.0.0.0:0")?;
    let target = SocketAddrV4::new(Ipv4Addr::new(239, 0, 0, 1), 39191);
    let msg = b"hello multicast 3.2.1";
    socket.send_to(msg, target)?;
    println!("sent to {target}");
    Ok(())
}
