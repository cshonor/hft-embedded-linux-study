//! 替代 `repr`：C、transparent、整数枚举、packed、align。

use std::mem::{align_of, offset_of, size_of};

#[repr(C)]
pub struct CStruct {
    pub a: u8,
    pub b: u32,
}

#[repr(transparent)]
pub struct Wrapper(pub i32);

#[repr(u8)]
pub enum Status {
    Ok = 0,
    Err = 1,
}

#[repr(packed)]
pub struct Packed {
    pub a: u8,
    pub b: u32,
}

#[repr(align(64))]
pub struct CacheLinePadded(pub u8);

pub fn print_alternative_reprs() {
    println!("--- repr(C) ---");
    println!(
        "  CStruct: size {} a@{} b@{}",
        size_of::<CStruct>(),
        offset_of!(CStruct, a),
        offset_of!(CStruct, b),
    );

    println!("--- repr(transparent) ---");
    println!(
        "  Wrapper = {} B, inner i32 = {} B",
        size_of::<Wrapper>(),
        size_of::<i32>(),
    );

    println!("--- repr(u8) enum ---");
    println!("  Status = {} B", size_of::<Status>());

    println!("--- repr(packed) ---");
    println!(
        "  Packed: size {} (无字段间 padding；未对齐访问危险)",
        size_of::<Packed>(),
    );

    println!("--- repr(align(64)) ---");
    println!(
        "  CacheLinePadded: size {} align {}",
        size_of::<CacheLinePadded>(),
        align_of::<CacheLinePadded>(),
    );
}
