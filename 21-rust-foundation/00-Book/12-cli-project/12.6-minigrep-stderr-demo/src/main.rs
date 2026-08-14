use std::env;
use std::process;

use minigrep_stderr_demo::Config;

fn main() {
    let args: Vec<String> = env::args().collect();

    let config = Config::new(&args).unwrap_or_else(|err| {
        eprintln!("Problem parsing arguments: {err}");
        eprintln!("usage: cargo run -- <query> <filename>");
        process::exit(1);
    });

    if let Err(e) = minigrep_stderr_demo::run(config) {
        eprintln!("Application error: {e}");
        process::exit(1);
    }
}

