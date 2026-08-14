use std::path::PathBuf;

use clap::Parser;

#[derive(Debug, Parser)]
#[command(name = "cdn-edge", about = "Lightweight HTTP edge cache")]
pub struct Config {
    /// Listen port
    #[arg(long, default_value_t = 8080)]
    pub port: u16,

    /// Origin directory (local filesystem)
    #[arg(long, default_value = "./origin")]
    pub origin: PathBuf,

    /// In-memory cache capacity (number of objects)
    #[arg(long, default_value_t = 256)]
    pub cache_capacity: usize,
}
