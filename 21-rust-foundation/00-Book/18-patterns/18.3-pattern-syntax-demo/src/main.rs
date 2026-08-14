// 18.3 模式语法 demo
//   cargo run           — 全部语法整合
//   cargo run -- pitfalls — 易错点 + 编译报错对照

use pattern_syntax_demo::{demo_all_pattern_syntax, demo_compile_error_notes, demo_underscore_prefix_moves};

fn main() {
    let arg = std::env::args().nth(1);
    let mode = arg.as_deref().unwrap_or("full");

    if mode == "pitfalls" {
        println!("=== 18.3 易错点 & 编译报错对照 ===\n");
        demo_compile_error_notes();
        println!();
        demo_underscore_prefix_moves();
        println!("\nok: pitfalls demo 完成");
        return;
    }

    println!("=== 18.3 模式语法整合 Demo ===\n");
    demo_all_pattern_syntax();
    println!("\nok: pattern syntax demo 完成（-- pitfalls）");
}
