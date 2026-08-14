// 19.4 高级函数与闭包 demo
//   cargo run           — 完整 demo
//   cargo run -- pitfalls — 易错点对照

use advanced_functions_closures_demo::{
    demo_all_functions_closures, demo_compile_error_notes,
};

fn main() {
    let arg = std::env::args().nth(1);
    let mode = arg.as_deref().unwrap_or("full");

    if mode == "pitfalls" {
        println!("=== 19.4 易错点 & 易混点对照 ===\n");
        demo_compile_error_notes();
        println!("\nok: pitfalls demo 完成");
        return;
    }

    println!("=== 19.4 高级函数与闭包 Demo ===\n");
    demo_all_functions_closures();
    println!("\nok: functions & closures demo 完成（-- pitfalls）");
}
