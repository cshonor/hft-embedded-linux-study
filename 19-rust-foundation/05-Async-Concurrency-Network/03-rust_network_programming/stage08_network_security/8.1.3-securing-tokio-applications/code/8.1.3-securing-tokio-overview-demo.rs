//! §8.1.3：Tokio TLS 升级路径。

fn main() {
    println!("Tokio TLS stack:");
    println!("  TcpListener::accept().await");
    println!("  tls_acceptor.accept(stream).await  // tokio-rustls");
    println!("  hyper/axum serve on TlsStream");
    println!("Book: tokio-tls + native-tls often needs PKCS#12 (.pfx)");
}
