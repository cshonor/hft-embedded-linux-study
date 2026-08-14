//! §4.1.1：`#[serde(rename)]` 自定义 JSON 字段名。
use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize)]
struct Point {
    #[serde(rename = "custom_x")]
    x: f64,
    #[serde(rename = "custom_y")]
    y: f64,
}

fn main() -> serde_json::Result<()> {
    let p = Point { x: 3.0, y: 4.0 };
    let json = serde_json::to_string(&p)?;
    println!("{json}");
    assert!(json.contains("custom_x"));
    let back: Point = serde_json::from_str(&json)?;
    println!("back: ({}, {})", back.x, back.y);
    Ok(())
}
