//! 禁止返回 DST 类型 `str`。
//! 预期：**编译失败**（unsized return type）。
//!
//! ```bash
//! rustc --edition 2021 compile_fail/return_str_dst.rs
//! ```

fn bad() -> str {
    "hello"
}

fn main() {
    let _ = bad();
}
