pub struct Settings {
    pub host: String,
    pub port: u16,
}

impl Settings {
    pub fn load() -> Self {
        Settings {
            host: "127.0.0.1".into(),
            port: 8080,
        }
    }
}
