use std::net::IpAddr;

fn main() {
    let mut args = std::env::args().skip(1);
    let mode = args.next().unwrap_or_else(|| "help".to_string());

    match mode.as_str() {
        "ip" => {
            // 你比编译器知道更多：硬编码字符串必然是合法 IP，这里 unwrap 是可接受的
            let home: IpAddr = "127.0.0.1".parse().unwrap();
            println!("home ip = {home}");
        }
        "parse" => {
            // 用户输入是可预期会出错的：返回 Result / 显式处理更合适
            let s = args.next().unwrap_or_else(|| "help".to_string());
            match parse_u32(&s) {
                Ok(n) => println!("parsed u32 = {n}"),
                Err(e) => println!("parse error: {e}"),
            }
        }
        "guess" => {
            let s = args.next().unwrap_or_else(|| "help".to_string());
            let n: i32 = match s.parse() {
                Ok(v) => v,
                Err(_) => {
                    eprintln!("guess expects an integer, got {s:?}");
                    return;
                }
            };

            // 契约：Guess 只能是 1..=100；越界代表调用者 bug → panic
            let g = Guess::new(n);
            println!("guess ok: {}", g.value());
        }
        _ => {
            eprintln!(
                "usage:\n  cargo run -- ip\n  cargo run -- parse <u32>\n  cargo run -- guess <1..=100>"
            );
        }
    }
}

fn parse_u32(s: &str) -> Result<u32, String> {
    s.parse::<u32>().map_err(|e| e.to_string())
}

pub struct Guess {
    value: i32,
}

impl Guess {
    pub fn new(value: i32) -> Guess {
        if value < 1 || value > 100 {
            panic!("Guess value must be between 1 and 100, got {value}.");
        }
        Guess { value }
    }

    pub fn value(&self) -> i32 {
        self.value
    }
}

