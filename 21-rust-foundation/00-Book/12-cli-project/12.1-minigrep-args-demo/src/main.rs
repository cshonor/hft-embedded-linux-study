use std::env;

fn main() {
    let args: Vec<String> = env::args().collect();
    println!("{args:?}");

    if args.len() < 3 {
        eprintln!("usage: cargo run -- <query> <filename>");
        return;
    }

    let query = &args[1];
    let filename = &args[2];

    println!("Searching for {query}");
    println!("In file {filename}");
}

