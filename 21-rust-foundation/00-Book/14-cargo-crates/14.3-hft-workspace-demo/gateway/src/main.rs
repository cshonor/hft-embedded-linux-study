use engine::on_tick;
use market_data::decode;

fn main() {
    let tick = decode(b"BTCUSDT");
    on_tick(&tick.symbol, tick.price);
    println!("[gateway] exe running (workspace: gateway → engine + market_data)");
}
