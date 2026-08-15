//! §9.5：`bitflags` + `url`。
use bitflags::bitflags;
use url::Url;

bitflags! {
    struct TcpFlags: u8 {
        const FIN = 0b001;
        const SYN = 0b010;
        const ACK = 0b100;
    }
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let flags = TcpFlags::SYN | TcpFlags::ACK;
    println!("TCP flags bits: {:b}, has ACK? {}", flags.bits(), flags.contains(TcpFlags::ACK));

    let u = Url::parse("https://example.com:8443/path?q=1#frag")?;
    println!("scheme={} host={:?} path={}", u.scheme(), u.host(), u.path());
    Ok(())
}
