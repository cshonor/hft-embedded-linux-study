//! §6.4 — 本章小结（指向同章其它 demo）。

fn main() {
    println!("6.4-summary — Chapter 6 demos");
    println!("  cargo run --bin demo_6_1_raw_http   # 6.1 std::net 最小 HTTP");
    println!("  cargo run --bin demo_6_3_reqwest_get # 6.3 reqwest GET（外网）");
    println!("  cargo run --manifest-path .../6.2-introducing-rocket/Cargo.toml --bin demo_6_2_rocket_hello");
}
