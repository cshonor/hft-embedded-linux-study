pub struct Tick {
    pub symbol: String,
    pub price: f64,
}

pub fn decode(raw: &[u8]) -> Tick {
    let s = std::str::from_utf8(raw).unwrap_or("UNKNOWN");
    Tick {
        symbol: s.to_string(),
        price: 100.0,
    }
}
