//! Item 18: Result 优先；catch_unwind 不是业务错误恢复

use std::num::NonZeroI64;
use std::panic;

#[derive(Debug, PartialEq)]
enum DivideError {
    DivideByZero,
}

fn divide(a: i64, b: i64) -> Result<i64, DivideError> {
    if b == 0 {
        Err(DivideError::DivideByZero)
    } else {
        Ok(a / b)
    }
}

/// 反模式：用 panic + catch_unwind 当除零恢复（panic=abort 时无效）
fn divide_recover_bad(a: i64, b: i64, default: i64) -> i64 {
    let result = panic::catch_unwind(|| {
        let b = NonZeroI64::new(b).expect("zero");
        a / b.get()
    });
    match result {
        Ok(x) => x,
        Err(_) => default,
    }
}

fn main() {
    println!("=== 正路：Result ===");
    match divide(10, 0) {
        Ok(n) => println!("ok = {n}"),
        Err(e) => println!("err = {e:?}"),
    }

    println!("\n=== 反模式：catch_unwind（演示用；生产请用 Result）===");
    println!(
        "divide_recover_bad(10, 0, -1) = {}",
        divide_recover_bad(10, 0, -1)
    );
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn result_path() {
        assert_eq!(divide(10, 2), Ok(5));
        assert_eq!(divide(1, 0), Err(DivideError::DivideByZero));
    }
}
