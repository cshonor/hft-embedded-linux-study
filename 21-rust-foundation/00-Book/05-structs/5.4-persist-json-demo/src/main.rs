//! model/ 放 struct；main 里模拟 db 层：序列化写文件、读回反序列化。

mod model;

use model::Trade;
use std::fs;
use std::path::Path;

const TRADE_FILE: &str = "trade.json";

fn save_trade(path: &Path, t: &Trade) -> std::io::Result<()> {
    let json = serde_json::to_string_pretty(t).expect("serialize");
    fs::write(path, json)
}

fn load_trade(path: &Path) -> std::io::Result<Trade> {
    let json = fs::read_to_string(path)?;
    serde_json::from_str(&json).map_err(|e| std::io::Error::new(std::io::ErrorKind::InvalidData, e))
}

fn main() {
    let t = Trade {
        order_id: 10001,
        code: "600000".into(),
        price: 10.25,
        volume: 500,
        timestamp: 1_700_000_000_000,
    };

    println!("=== 1. 内存 struct ===\n{t:?}");

    save_trade(Path::new(TRADE_FILE), &t).expect("write trade.json");
    println!("\n=== 2. 已写入 {TRADE_FILE} ===");
    println!("{}", fs::read_to_string(TRADE_FILE).unwrap());

    let back = load_trade(Path::new(TRADE_FILE)).expect("read trade.json");
    println!("\n=== 3. 读回 struct ===\n{back:?}");
    assert_eq!(t, back);
}
