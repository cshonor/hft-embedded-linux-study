//! Normalized OHLCV bar (mid-low frequency).

#[derive(Debug, Clone, Copy)]
pub struct Bar {
    pub symbol: &'static str,
    pub open: f64,
    pub high: f64,
    pub low: f64,
    pub close: f64,
    pub volume: f64,
}

pub struct MockFeed;

impl MockFeed {
    pub fn sample_bars() -> Vec<Bar> {
        vec![
            Bar {
                symbol: "DEMO",
                open: 100.0,
                high: 101.0,
                low: 99.5,
                close: 100.5,
                volume: 1_000.0,
            },
            Bar {
                symbol: "DEMO",
                open: 100.5,
                high: 102.0,
                low: 100.0,
                close: 101.5,
                volume: 1_200.0,
            },
        ]
    }
}
