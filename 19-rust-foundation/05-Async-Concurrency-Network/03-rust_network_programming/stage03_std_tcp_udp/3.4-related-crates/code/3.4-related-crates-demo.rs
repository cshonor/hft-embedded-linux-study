//! §3.4：第三方 crate 选型速查（无依赖）。

fn main() {
    println!("3.4 related crates (see notes for details):");
    println!("  ipnetwork  — CIDR / prefix");
    println!("  mio        — poll + non-blocking → Tokio");
    println!("  libpnet    — link-layer / sniffing");
    println!("  hickory-dns — full DNS (successor to trust-dns)");
}
