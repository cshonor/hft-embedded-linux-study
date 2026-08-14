//! Item 20: 拥有权 `Vec<u8>` 换灵活性，避免 `Tlv<'a>` 生命周期传染

#[derive(Debug, Clone, PartialEq)]
struct Tlv {
    tag: u8,
    value: Vec<u8>,
}

fn parse_tlv(input: &[u8]) -> Option<Tlv> {
    if input.len() < 2 {
        return None;
    }
    let tag = input[0];
    let len = input[1] as usize;
    if input.len() < 2 + len {
        return None;
    }
    Some(Tlv {
        tag,
        value: input[2..2 + len].to_vec(),
    })
}

struct NetworkServer {
    cached: Option<Tlv>,
}

impl NetworkServer {
    fn cache_from_packet(&mut self, packet: &[u8]) {
        self.cached = parse_tlv(packet);
    }
}

fn main() {
    let mut server = NetworkServer { cached: None };

    for round in 0..2 {
        let data = vec![0x01, 0x03, b'a', b'b', b'c'];
        server.cache_from_packet(&data);
        println!("round {round}: cached = {:?}", server.cached);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_and_cache() {
        let mut s = NetworkServer { cached: None };
        let pkt = vec![0x02, 0x02, b'x', b'y'];
        s.cache_from_packet(&pkt);
        assert_eq!(
            s.cached,
            Some(Tlv {
                tag: 0x02,
                value: vec![b'x', b'y'],
            })
        );
    }
}
