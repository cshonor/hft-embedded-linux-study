// 19.2 高级 trait demo
//   cargo run           — 五块完整 demo
//   cargo run -- pitfalls — 易错点对照

use advanced_traits_demo::{demo_all_advanced_traits, demo_compile_error_notes};

fn main() {
    let arg = std::env::args().nth(1);
    let mode = arg.as_deref().unwrap_or("full");

    if mode == "pitfalls" {
        println!("=== 19.2 易错点 & 编译报错对照 ===\n");
        demo_compile_error_notes();
        println!("\nok: pitfalls demo 完成");
        return;
    }

    println!("=== 19.2 高级 Trait 完整 Demo ===\n");
    demo_all_advanced_traits();
    println!("\nok: advanced traits demo 完成（-- pitfalls）");
}
