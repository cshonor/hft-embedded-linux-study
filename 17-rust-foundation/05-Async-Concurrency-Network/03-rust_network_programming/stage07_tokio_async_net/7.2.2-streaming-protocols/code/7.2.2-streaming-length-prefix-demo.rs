//! §7.2.2：长度前缀帧（大端 u32 + body）。
fn decode_frame(buf: &[u8]) -> Option<(&[u8], &[u8])> {
    if buf.len() < 4 {
        return None;
    }
    let len = u32::from_be_bytes(buf[..4].try_into().ok()?) as usize;
    if buf.len() < 4 + len {
        return None;
    }
    Some((&buf[4 + len..], &buf[4..4 + len]))
}

fn main() {
    let payload = b"{\"header\":\"ok\",\"body\":\"streaming...\"}";
    let mut wire = Vec::new();
    wire.extend_from_slice(&(payload.len() as u32).to_be_bytes());
    wire.extend_from_slice(payload);
    if let Some((rest, frame)) = decode_frame(&wire) {
        println!("header frame: {}", String::from_utf8_lossy(frame));
        println!("remaining bytes: {}", rest.len());
    }
}
