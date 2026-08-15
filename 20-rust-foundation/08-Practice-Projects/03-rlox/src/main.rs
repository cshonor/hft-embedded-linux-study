mod error;
mod scanner;
mod token;

use std::io::{self, Write};

use error::LoxError;
use scanner::Scanner;

fn main() {
    let args: Vec<String> = std::env::args().collect();
    if args.len() == 1 || (args.len() > 1 && args[1] == "repl") {
        if let Err(e) = repl() {
            eprintln!("{e}");
            std::process::exit(1);
        }
    } else {
        eprintln!("usage: rlox [repl]");
        std::process::exit(1);
    }
}

fn repl() -> Result<(), LoxError> {
    println!("rlox repl (scanner only — parser/interpreter TODO). type 'exit' to quit.");
    let stdin = io::stdin();
    loop {
        print!("> ");
        io::stdout().flush().ok();
        let mut line = String::new();
        if stdin.read_line(&mut line)? == 0 {
            break;
        }
        let line = line.trim();
        if line.is_empty() {
            continue;
        }
        if line == "exit" {
            break;
        }
        let tokens = Scanner::new(line).scan_tokens()?;
        for tok in tokens {
            println!("  {tok:?}");
        }
    }
    Ok(())
}
