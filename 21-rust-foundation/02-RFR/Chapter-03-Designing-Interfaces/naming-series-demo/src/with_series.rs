#[derive(Debug, PartialEq)]
struct ServerConfig {
    host: String,
    port: u16,
    workers: usize,
    tls: bool,
}

struct ServerConfigBuilder {
    host: String,
    port: u16,
    workers: usize,
    tls: bool,
}

impl ServerConfigBuilder {
    fn new(host: impl Into<String>) -> Self {
        Self {
            host: host.into(),
            port: 8080,
            workers: 4,
            tls: false,
        }
    }

    fn with_port(mut self, port: u16) -> Self {
        self.port = port;
        self
    }

    fn with_workers(mut self, n: usize) -> Self {
        self.workers = n;
        self
    }

    fn with_tls(mut self, enable: bool) -> Self {
        self.tls = enable;
        self
    }

    fn build(self) -> ServerConfig {
        ServerConfig {
            host: self.host,
            port: self.port,
            workers: self.workers,
            tls: self.tls,
        }
    }
}

pub fn run() {
    let mut v = Vec::with_capacity(100);
    assert_eq!(v.len(), 0);
    assert!(v.capacity() >= 100);
    v.push(1);
    assert_eq!(v.len(), 1);

    let cfg = ServerConfigBuilder::new("127.0.0.1")
        .with_port(443)
        .with_tls(true)
        .with_workers(8)
        .build();

    assert_eq!(
        cfg,
        ServerConfig {
            host: "127.0.0.1".into(),
            port: 443,
            workers: 8,
            tls: true,
        }
    );

    println!("01-5 with_ ok");
}
