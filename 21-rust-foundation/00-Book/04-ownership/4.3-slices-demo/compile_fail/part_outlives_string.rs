//! `&str` 借用的堆 `String` 不能比 `String` 活得更久。
//! 预期：**编译失败**（生命周期 E0597）。
//!
//! ```bash
//! rustc --edition 2021 compile_fail/part_outlives_string.rs
//! ```

fn main() {
    let part: &str;
    {
        let st = String::from("temporary");
        part = &st[..];
    }
    println!("{part}");
}
