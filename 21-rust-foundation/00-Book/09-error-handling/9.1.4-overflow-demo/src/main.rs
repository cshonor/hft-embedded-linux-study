// 9.1.4 整数溢出 demo

fn show_apis() {
    println!("=== 显式 API（dev / release 行为相同）===");
    println!("0u8.wrapping_sub(1)    = {}", 0u8.wrapping_sub(1));
    println!("255u8.wrapping_add(1)  = {}", 255u8.wrapping_add(1));
    println!("0u8.saturating_sub(1)  = {}", 0u8.saturating_sub(1));
    println!("255u8.saturating_add(1)= {}", 255u8.saturating_add(1));

    match 1u32.checked_add(u32::MAX) {
        Some(v) => println!("1 + MAX = {v}"),
        None => println!("1 + MAX → None（溢出）"),
    }

    let min = i32::MIN;
    println!("i32::MIN.wrapping_sub(1) = {}", min.wrapping_sub(1));
    let max = i32::MAX;
    println!("i32::MAX.wrapping_add(1) = {}", max.wrapping_add(1));
}

fn show_plain() {
    println!("=== 裸运算 x -= 1（dev panic / release wrap）===");
    let mut x: u8 = 0;
    x -= 1; // dev: panic · release: 255
    println!("x = {x}");
}

fn main() {
    let mode = std::env::args().nth(1).unwrap_or_else(|| "help".to_string());
    match mode.as_str() {
        "api" => show_apis(),
        "plain" => show_plain(),
        _ => eprintln!("usage:\n  cargo run -- api\n  cargo run -- plain\n  cargo run --release -- plain"),
    }
}

#[cfg(test)]
mod tests {
    #[test]
    fn wrapping_u8() {
        assert_eq!(0u8.wrapping_sub(1), 255);
        assert_eq!(255u8.wrapping_add(1), 0);
    }

    #[test]
    fn checked_overflow_is_none() {
        assert!(1u32.checked_add(u32::MAX).is_none());
    }
}
