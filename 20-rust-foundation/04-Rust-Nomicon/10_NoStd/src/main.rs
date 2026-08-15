//! std 宿主：调用同一 crate 的 no_std 兼容 API。

fn main() {
    println!("=== no_std-compatible API (built with std) ===");
    println!("add(40, 2) = {}", nomicon_10_no_std::add(40, 2));
    println!("checksum(b'nomicon') = {}", nomicon_10_no_std::checksum(b"nomicon"));
    println!();
    println!("Build pure no_std lib:");
    println!("  cargo build --no-default-features");
    println!("See templates/no_main_linux.md for no_main executable.");
}
