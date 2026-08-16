//! §8.1.1：ACME / Let's Encrypt 流程速查。

fn main() {
    println!("ACME (Let's Encrypt) flow:");
    println!("  1. prove domain control (HTTP-01 or DNS-01)");
    println!("  2. receive cert + chain (fullchain.pem)");
    println!("  3. private key (privkey.pem)");
    println!("  4. auto renew before expiry (~90 days)");
    println!("Rust: acme-client, certbot, or cloud LB managed certs");
}
