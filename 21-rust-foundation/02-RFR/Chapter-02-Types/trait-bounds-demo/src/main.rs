//! RFR ch02 §08：Trait bounds · 静态/动态分发 · HRTB

use std::error::Error;
use std::fmt::Display;

// --- 静态：impl Trait ---

fn print_static(x: impl Display) {
    println!("static: {x}");
}

// --- 静态：显式 T（两参数同类型）---

fn print_pair<T: Display>(a: T, b: T) {
    println!("pair: {a} | {b}");
}

// --- 动态：dyn Trait ---

fn log_dyn(e: &dyn Error) {
    eprintln!("dyn err: {e}");
}

// --- HRTB ---

fn takes_any_str_closure<F>(f: F)
where
    F: for<'a> Fn(&'a str),
{
    f("literal");
    let owned = String::from("owned");
    f(&owned);
}

// 过窄签名（勿启用 — 仅作对照注释）：
// fn takes_narrow<'a, F>(f: F)
// where
//     F: Fn(&'a str),
// {
//     f("only one 'a");
// }

#[derive(Debug)]
struct AppError(&'static str);

impl std::fmt::Display for AppError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.0)
    }
}

impl Error for AppError {}

fn main() {
    print_static(42);
    print_static("hello");

    print_pair(1, 2);

    let errs: Vec<Box<dyn Error>> = vec![
        Box::new(AppError("io")),
        Box::new(std::io::Error::new(std::io::ErrorKind::Other, "net")),
    ];
    for e in &errs {
        log_dyn(e.as_ref());
    }

    takes_any_str_closure(|s| println!("HRTB: {s}"));
}
