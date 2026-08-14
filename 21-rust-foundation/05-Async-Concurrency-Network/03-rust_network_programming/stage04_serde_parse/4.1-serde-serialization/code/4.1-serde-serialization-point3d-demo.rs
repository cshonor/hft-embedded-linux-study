//! §4.1：Point3D JSON 往返。
use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize, PartialEq)]
struct Point3D {
    x: f64,
    y: f64,
    z: f64,
}

fn main() -> serde_json::Result<()> {
    let p = Point3D {
        x: 1.0,
        y: 2.0,
        z: 2.0,
    };
    let json = serde_json::to_vec(&p)?;
    let back: Point3D = serde_json::from_slice(&json)?;
    assert_eq!(p, back);
    let dist = (p.x * p.x + p.y * p.y + p.z * p.z).sqrt();
    println!("json bytes: {}", String::from_utf8_lossy(&json));
    println!("distance from origin: {dist:.4}");
    Ok(())
}
