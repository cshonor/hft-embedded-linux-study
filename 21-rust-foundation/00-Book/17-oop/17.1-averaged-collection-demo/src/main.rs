// 17.1 面向对象三大特征 demo
//   cargo run         — 对象 + 封装 + 多态整合
//   cargo run -- notes — 传统 OOP vs Rust 对照

use averaged_collection_demo::{demo_all_oop_features, demo_traditional_vs_rust_notes};

fn main() {
    let arg = std::env::args().nth(1);
    let mode = arg.as_deref().unwrap_or("full");

    if mode == "notes" {
        println!("=== 17.1 传统 OOP vs Rust ===\n");
        demo_traditional_vs_rust_notes();
        println!("\nok: notes demo 完成");
        return;
    }

    println!("=== 17.1 面向对象三大特征 ===\n");
    demo_all_oop_features();
    println!("\nok: oop demo 完成（-- notes）");
}
