//! §7.2.3：Tokio 生态速查。

fn main() {
    println!("Tokio ecosystem (modern):");
    println!("  tokio::net, tokio::io  — async I/O");
    println!("  reqwest               — HTTP client");
    println!("  tokio-rustls          — TLS");
    println!("  hyper / axum          — HTTP server");
    println!("  tonic                 — gRPC");
    println!("  tower                 — middleware");
    println!("Deep dive: ../../02-async_tokio/");
}
