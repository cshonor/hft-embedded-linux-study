mod engine;
mod market;
mod oms;
mod risk;
mod strategy;

use tracing_subscriber::EnvFilter;

#[tokio::main]
async fn main() {
    tracing_subscriber::fmt()
        .with_env_filter(EnvFilter::from_default_env().add_directive("midfreq_trading_bot=info".parse().unwrap()))
        .init();

    tracing::info!("midfreq-trading-bot — framework skeleton");
    engine::run_demo().await;
}
