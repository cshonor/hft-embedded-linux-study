// 17.2 Trait 对象 demo
//   cargo run            — GUI Draw 完整 demo（含扩展 SelectBox）
//   cargo run -- compare   — 枚举 vs 泛型 vs trait 对象
//   cargo run -- safety    — 对象安全规则
//   cargo run -- pitfalls  — 易错点 + 编译报错对照

use trait_objects_gui_demo::{
    demo_compile_error_notes, demo_enum_vs_dyn_notes, demo_generic_vs_dyn_notes,
    demo_gui_trait_objects, demo_gui_with_extension, demo_mutable_trait_object,
    demo_object_safety_notes, Draw, Screen,
};

/// 使用者在 main 中扩展的类型 — 无需修改 Screen / Draw 库代码
struct SelectBox;

impl Draw for SelectBox {
    fn draw(&self) {
        println!("  绘制下拉选择框");
    }
}

fn main() {
    let arg = std::env::args().nth(1);
    let mode = arg.as_deref().unwrap_or("full");

    if mode == "compare" {
        println!("=== 17.2 枚举 vs 泛型 vs Trait 对象 ===\n");
        demo_enum_vs_dyn_notes();
        demo_generic_vs_dyn_notes();
        println!("\nok: compare demo 完成");
        return;
    }

    if mode == "safety" {
        println!("=== 17.2 对象安全（Object Safe）===\n");
        demo_object_safety_notes();
        println!("\nok: safety demo 完成");
        return;
    }

    if mode == "pitfalls" {
        println!("=== 17.2 易错点 & 编译报错对照 ===\n");
        demo_compile_error_notes();
        println!();
        demo_mutable_trait_object();
        println!("\nok: pitfalls demo 完成");
        return;
    }

    println!("=== 17.2 GUI Trait 对象 Demo ===\n");
    demo_gui_trait_objects();

    demo_gui_with_extension(|screen| {
        screen.add_component(Box::new(SelectBox));
    });

    println!("\n--- 可变 trait 对象 ---");
    demo_mutable_trait_object();

    println!("\nok: trait objects demo 完成（-- compare / -- safety / -- pitfalls）");
}
