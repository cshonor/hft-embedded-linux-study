//! 故意写错：活跃 `&str` 存在时调用 `s.clear()`。
//! 预期：**编译失败**（借用检查 E0502）。
//!
//! ```bash
//! rustc --edition 2021 compile_fail/slice_blocks_clear.rs
//! ```

fn first_word(s: &str) -> &str {
    let bytes = s.as_bytes();
    for (i, &b) in bytes.iter().enumerate() {
        if b == b' ' {
            return &s[0..i];
        }
    }
    &s[..]
}

fn main() {
    let mut s = String::from("hello world");
    let word = first_word(&s);
    s.clear(); // error: cannot borrow `s` as mutable because it is also borrowed as immutable
    println!("{word}");
}
