//! §4.2：用 `nom` 解析 HTTP 请求行。
use nom::{
    bytes::complete::{tag, take_until1},
    sequence::tuple,
    IResult,
};

fn parse_request_line(input: &str) -> IResult<&str, (&str, &str, &str)> {
    let (input, (method, _, path, _, version)) = tuple((
        take_until1(" "),
        tag(" "),
        take_until1(" "),
        tag(" "),
        take_until1("\r\n"),
    ))(input)?;
    Ok((input, (method, path, version)))
}

fn main() {
    let line = "GET /api/users HTTP/1.1\r\n";
    match parse_request_line(line) {
        Ok((rest, (method, path, version))) => {
            println!("method={method} path={path} version={version} rest={rest:?}");
        }
        Err(e) => eprintln!("parse error: {e:?}"),
    }
}
