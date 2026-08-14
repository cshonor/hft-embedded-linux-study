#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(feature = "alloc")]
extern crate alloc;

#[cfg(feature = "alloc")]
use alloc::vec::Vec;

#[derive(Debug, PartialEq)]
pub struct Packet {
    pub tag: u8,
    #[cfg(feature = "alloc")]
    pub payload: Vec<u8>,
}

#[cfg(feature = "alloc")]
pub fn parse(tag: u8, data: &[u8]) -> Result<Packet, &'static str> {
    let mut payload = Vec::new();
    payload.try_reserve(data.len()).map_err(|_| "oom")?;
    payload.extend_from_slice(data);
    Ok(Packet { tag, payload })
}

#[cfg(all(test, feature = "alloc"))]
mod tests {
    use super::*;

    #[test]
    fn parse_packet() {
        let p = parse(1, b"hi").unwrap();
        assert_eq!(p.tag, 1);
        assert_eq!(p.payload, b"hi");
    }
}
