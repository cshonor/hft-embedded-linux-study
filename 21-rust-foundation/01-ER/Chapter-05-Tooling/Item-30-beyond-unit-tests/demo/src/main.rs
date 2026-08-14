//! Item 30: std::hint::black_box 防止编译期常量折叠

fn factorial(n: u64) -> u64 {
    (1..=n).product()
}

fn main() {
    let n = std::hint::black_box(15);
    let result = factorial(n);
    assert_eq!(result, 1_307_674_368_000);
    println!("factorial({n}) = {result}");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn factorial_with_black_box() {
        let n = std::hint::black_box(10);
        assert_eq!(factorial(n), 3_628_800);
    }
}
