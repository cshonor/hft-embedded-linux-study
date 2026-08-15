// 19.5 声明宏 demo
//   cargo run           — my_vec! 输出
//   cargo run -- pitfalls — 易错点对照
//
// 过程宏 derive 示例：cargo run -p pancakes（workspace 根目录）

use decl_macro_demo::{demo_compile_error_notes, demo_decl_macro};

fn main() {
    let arg = std::env::args().nth(1);
    let mode = arg.as_deref().unwrap_or("full");

    if mode == "pitfalls" {
        println!("=== 19.5 宏易错点对照 ===\n");
        demo_compile_error_notes();
        println!("\n过程宏 demo: cargo run -p pancakes");
        println!("ok: pitfalls 完成");
        return;
    }

    println!("=== 19.5 声明宏 macro_rules! Demo ===\n");
    demo_decl_macro();
    println!("\n过程宏 demo: cargo run -p pancakes");
    println!("ok: decl_macro demo 完成（-- pitfalls）");
}
