//! §8.1.2：PEM / PKCS#12 与自签名命令速查。

fn main() {
    println!("OpenSSL dev cert (concept):");
    println!("  openssl req -x509 -newkey rsa:3072 -nodes -days 365 -keyout key.pem -out cert.pem");
    println!("  openssl pkcs12 -export -in cert.pem -inkey key.pem -out server.pfx");
    println!("rust-openssl: generate X.509 in code for integration tests");
}
