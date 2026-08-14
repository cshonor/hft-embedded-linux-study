// 18.2 可反驳性 demo
//   cargo run         — 完整整合（六场景）
//   cargo run -- errors — 合法/非法对照说明

use refutability_demo::{
    demo_all_pattern_sites, demo_irrefutable_if_let_warning, demo_refutable_vs_irrefutable_notes,
};

fn main() {
    let arg = std::env::args().nth(1);
    let mode = arg.as_deref().unwrap_or("full");

    if mode == "errors" {
        println!("=== 18.2 可反驳 / 不可反驳对照 ===\n");
        demo_refutable_vs_irrefutable_notes();
        demo_irrefutable_if_let_warning();
        println!("\nok: errors demo 完成");
        return;
    }

    println!("=== 18.2 完整整合：模式六场景 ===\n");
    demo_all_pattern_sites();
    println!("\nok: refutability demo 完成（-- errors）");
}
