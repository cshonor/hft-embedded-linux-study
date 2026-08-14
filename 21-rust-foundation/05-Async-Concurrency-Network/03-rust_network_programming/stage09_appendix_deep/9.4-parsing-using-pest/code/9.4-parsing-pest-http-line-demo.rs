//! §9.4：Pest 解析 HTTP 请求行（对照 Ch4 nom）。
use pest::Parser;
use pest_derive::Parser;

#[derive(Parser)]
#[grammar = "9.4-parsing-using-pest/http_line.pest"]
struct HttpLineParser;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let input = "GET /api/users HTTP/1.1";
    let mut pairs = HttpLineParser::parse(Rule::request, input)?;
    let request = pairs.next().unwrap();
    let mut inner = request.into_inner();
    let method = inner.next().unwrap().as_str();
    let path = inner.next().unwrap().as_str();
    let version = inner.next().unwrap().as_str();
    println!("pest: {method} {path} {version}");
    Ok(())
}
