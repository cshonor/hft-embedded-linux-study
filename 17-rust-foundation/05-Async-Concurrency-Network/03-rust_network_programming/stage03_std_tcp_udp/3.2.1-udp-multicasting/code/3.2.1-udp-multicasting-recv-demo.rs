//! §3.2.1：加入 IPv4 多播组并接收（组 `239.0.0.1:39191`）。
use std::net::{Ipv4Addr, SocketAddrV4, UdpSocket};
use std::time::Duration;

const MULTICAST: Ipv4Addr = Ipv4Addr::new(239, 0, 0, 1);
const PORT: u16 = 39191;

fn main() -> std::io::Result<()> {
    let bind_addr = SocketAddrV4::new(Ipv4Addr::UNSPECIFIED, PORT);
    let socket = UdpSocket::bind(bind_addr)?;
    socket.join_multicast_v4(&MULTICAST, &Ipv4Addr::UNSPECIFIED)?;
    socket
        .set_read_timeout(Some(Duration::from_secs(30)))
        .ok();
    println!(
        "listening multicast {MULTICAST}:{PORT} (30s timeout); send with demo_3_2_1_multicast_send"
    );
    let mut buf = [0u8; 1500];
    match socket.recv_from(&mut buf) {
        Ok((len, src)) => println!("from {src}: {}", String::from_utf8_lossy(&buf[..len])),
        Err(e) => println!("recv: {e} (no sender is OK for learning API)"),
    }
    Ok(())
}
