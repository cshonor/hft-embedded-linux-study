//! JSON 默认 UTF-8；与 GB2312/GBK 客户端互转时在字节层用 encoding_rs。

use encoding_rs::GBK;
use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize, PartialEq)]
struct Resp {
    name: String,
    msg: String,
}

fn main() {
    println!("=== 1. 标准路径：Rust String -> JSON UTF-8（无需转码）===");
    let data = Resp {
        name: "张三".into(),
        msg: "后端返回内容".into(),
    };
    let json_utf8 = serde_json::to_vec(&data).unwrap();
    let back: Resp = serde_json::from_slice(&json_utf8).unwrap();
    println!("JSON UTF-8: {}", String::from_utf8_lossy(&json_utf8));
    assert_eq!(data, back);

    println!("\n=== 2. UTF-8 字符串 <-> GBK 字节（GB2312 常用场景可用 GBK）===");
    let s = "测试中文内容";
    let (gb_bytes, _, enc_err) = GBK.encode(s);
    assert!(!enc_err, "encode should succeed for common simplified Chinese");
    println!("GBK bytes len = {}, sample = {:?}", gb_bytes.len(), &gb_bytes[..4.min(gb_bytes.len())]);

    let (utf8_cow, _, dec_err) = GBK.decode(&gb_bytes);
    assert!(!dec_err);
    println!("decode back: {}", utf8_cow);

    println!("\n=== 3. JSON UTF-8 整体 -> GBK body（老旧客户端）===");
    let json_str = std::str::from_utf8(&json_utf8).unwrap();
    let (json_gb_body, _, _) = GBK.encode(json_str);
    println!("JSON as GBK body len = {}", json_gb_body.len());
    // 对外: Content-Type: application/json; charset=GBK

    println!("\n=== 4. 收到 GBK JSON body -> 解析 ====");
    let (utf8_json, _, _) = GBK.decode(&json_gb_body);
    let parsed: Resp = serde_json::from_str(&utf8_json).unwrap();
    println!("parsed: {:?}", parsed);
    assert_eq!(data, parsed);
}
