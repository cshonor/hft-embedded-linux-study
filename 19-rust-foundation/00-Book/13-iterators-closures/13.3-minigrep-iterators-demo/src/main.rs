use std::env;
use std::process;

use minigrep_iterators_demo::Config;

fn main() {
    let config = Config::new(env::args()).unwrap_or_else(|err| {
        eprintln!("Problem parsing arguments: {err}");
        eprintln!("usage: cargo run -- <query> <filename>");
        process::exit(1);
    });

    if let Err(e) = minigrep_iterators_demo::run(config) {
        eprintln!("Application error: {e}");
        process::exit(1);
    }
}

