//! Item 3: Option ↔ Result 拓扑 — ok_or、transpose、and_then 链

use std::num::ParseIntError;

fn parse_positive(s: &str) -> Result<u32, ParseIntError> {
    s.parse::<u32>()
}

fn first_positive(nums: &[&str]) -> Option<Result<u32, ParseIntError>> {
    nums.iter().map(|s| parse_positive(s)).next()
}

fn first_positive_result(nums: &[&str]) -> Result<u32, String> {
    match first_positive(nums) {
        None => Err("slice is empty".into()),
        Some(r) => r.map_err(|e| e.to_string()),
    }
}

/// Option<Result<T,E>> → Result<Option<T>, E>
fn first_ok_only(nums: &[&str]) -> Result<Option<u32>, ParseIntError> {
    nums.iter()
        .map(|s| parse_positive(s))
        .find(|r| r.is_ok())
        .transpose()
}

fn main() {
    let nums = ["not-a-number", "42", "99"];

    println!(
        "find_map → Option<Result<_, _>>: {:?}",
        first_positive(&nums)
    );
    println!("ok_or  → Result: {:?}", first_positive_result(&nums));
    println!("transpose → Result<Option<_>>: {:?}", first_ok_only(&nums));

    let chain = parse_positive("7").map(|n| n * 2);
    println!("map chain: {:?}", chain);
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn transpose_skips_err_until_ok() {
        assert_eq!(first_ok_only(&["x", "3"]), Ok(Some(3)));
    }

    #[test]
    fn ok_or_on_empty() {
        assert!(first_positive_result(&[]).is_err());
    }
}
