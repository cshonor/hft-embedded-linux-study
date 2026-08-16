//! DST、ZST、空类型 (Void)。

use std::mem::size_of;

pub struct Nothing;

pub enum Void {}

pub fn infallible_ok() -> Result<u32, Void> {
    Ok(42)
}

pub fn print_exotic() {
    println!("--- ZST ---");
    println!("  size_of::<Nothing>() = {}", size_of::<Nothing>());
    println!("  size_of::<[Nothing; 1000]>() = {}", size_of::<[Nothing; 1000]>());

    println!("--- Void / Result<T, Void> ---");
    let _: Result<u32, Void> = infallible_ok();
    println!("  Result<u32, Void> 可表达「绝无 Err」");

    println!("--- DST / wide pointers ---");
    println!("  &u32 (thin)       = {} B", size_of::<&u32>());
    println!("  &[u8] (fat)      = {} B", size_of::<&[u8]>());
    println!("  &str (fat)       = {} B", size_of::<&str>());
    println!(
        "  &dyn Debug (fat) = {} B",
        size_of::<&dyn std::fmt::Debug>()
    );
    println!("  (fat = data ptr + len 或 vtable)");
}
