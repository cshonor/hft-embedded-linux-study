//! 默认 `repr(Rust)`：字段重排、padding、niche 优化。

use std::mem::{align_of, offset_of, size_of};

#[derive(Debug)]
pub struct RustLayout {
    pub a: u8,
    pub b: u32,
    pub c: u8,
}

#[repr(C)]
#[derive(Debug)]
pub struct CLayout {
    pub a: u8,
    pub b: u32,
    pub c: u8,
}

pub fn print_rust_vs_c() {
    println!("--- repr(Rust) vs repr(C) ---");
    println!(
        "  RustLayout: size {} align {} | a@{} b@{} c@{}",
        size_of::<RustLayout>(),
        align_of::<RustLayout>(),
        offset_of!(RustLayout, a),
        offset_of!(RustLayout, b),
        offset_of!(RustLayout, c),
    );
    println!(
        "  CLayout:    size {} align {} | a@{} b@{} c@{}",
        size_of::<CLayout>(),
        align_of::<CLayout>(),
        offset_of!(CLayout, a),
        offset_of!(CLayout, b),
        offset_of!(CLayout, c),
    );
    println!("  (Rust 可重排字段省 padding；C 按书写顺序 + C 对齐规则)");
}

pub fn print_niche() {
    println!("--- null pointer optimization ---");
    println!("  &u32         = {} B", size_of::<&u32>());
    println!("  Option<&u32> = {} B", size_of::<Option<&u32>>());
    println!("  u32          = {} B", size_of::<u32>());
    println!("  Option<u32>  = {} B", size_of::<Option<u32>>());
}
