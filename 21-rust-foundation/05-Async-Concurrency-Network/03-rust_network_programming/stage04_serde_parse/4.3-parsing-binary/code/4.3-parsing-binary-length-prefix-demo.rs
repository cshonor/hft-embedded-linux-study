//! §4.3：大端 u32 长度前缀 + payload（TCP 分帧常见形态）。
use nom::{bytes::complete::take, number::complete::be_u32, IResult};

fn parse_frame(input: &[u8]) -> IResult<&[u8], &[u8]> {
    let (input, len) = be_u32(input)?;
    take(len)(input)
}

fn main() {
    let payload = b"hello binary frame";
    let mut buf = Vec::new();
    buf.extend_from_slice(&(payload.len() as u32).to_be_bytes());
    buf.extend_from_slice(payload);

    match parse_frame(&buf) {
        Ok((rest, body)) => {
            println!("body: {}", String::from_utf8_lossy(body));
            println!("rest len: {}", rest.len());
        }
        Err(e) => eprintln!("{e:?}"),
    }
}
