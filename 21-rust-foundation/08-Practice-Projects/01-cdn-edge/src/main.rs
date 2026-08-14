mod cache;
mod config;
mod server;

use clap::Parser;
use config::Config;
use tracing_subscriber::EnvFilter;

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    tracing_subscriber::fmt()
        .with_env_filter(EnvFilter::from_default_env().add_directive("cdn_edge=info".parse()?))
        .init();

    let cfg = Config::parse();
    tracing::info!(port = cfg.port, origin = %cfg.origin.display(), "starting cdn-edge");
    server::run(cfg).await
}
