pub fn on_tick(symbol: &str, price: f64) {
    println!("[engine] tick {symbol} @ {price}");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn accepts_tick() {
        on_tick("BTC", 1.0);
    }
}
