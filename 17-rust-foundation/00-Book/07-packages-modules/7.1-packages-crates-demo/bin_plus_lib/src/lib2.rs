//! `src/lib2.rs` 不是第二个 Library Crate。
//!
//! Cargo 只认 `src/lib.rs` 为 lib crate root。
//! 本文件只有在 `lib.rs` 里 `mod lib2;` 时才是**同一 crate 内的模块**。
//!
//! ```bash
//! cd ../.. && cargo build -p bin_plus_lib_demo
//! ```

pub fn helper() -> &'static str {
    "same lib crate, different module file"
}
