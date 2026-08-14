// 19.3 高级类型 demo
//   cargo run           — 四大块整合 demo
//   cargo run -- pitfalls — 易错点对照

use advanced_types_demo::{demo_all_advanced_types, demo_compile_error_notes};

fn main() {
    let arg = std::env::args().nth(1);
    let mode = arg.as_deref().unwrap_or("full");

    if mode == "pitfalls" {
        println!("=== 19.3 易错点 & 编译报错对照 ===\n");
        demo_compile_error_notes();
        println!("\nok: pitfalls demo 完成");
        return;
    }

    println!("=== 19.3 高级类型完整 Demo ===\n");
    demo_all_advanced_types();
    println!("\nok: advanced types demo 完成（-- pitfalls）");
}
