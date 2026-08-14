//! 预期编译失败：变体名 V4 / Some 不能当类型标注（E0412）
enum IpAddr {
    V4(u8, u8, u8, u8),
    V6(String),
}

fn main() {
    let _ok: IpAddr = IpAddr::V4(127, 0, 0, 1);

    let ip: V4 = IpAddr::V4(127, 0, 0, 1);
    let x: Some<i32> = Some(5);
    let _ = (ip, x);
}
