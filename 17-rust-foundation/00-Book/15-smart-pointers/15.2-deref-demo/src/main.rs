// 15.2 Deref demo
//   cargo run           — 基础 + coercion + 方法调用
//   cargo run -- guards — MutexGuard
//   cargo run -- nested  — 多层 coercion
//   cargo run -- mut     — DerefMut + Box move-out
//   cargo run -- ref-target — 15.2.3 引用 vs 智能指针 + type Target
//   cargo run -- dot-paths  — 15.2.2 原生 & vs Deref 两条路线

use deref_demo::{
    demo_associated_type, demo_basic, demo_deref_mut, demo_dot_two_paths, demo_method_call,
    demo_mutex_guard, demo_nested_coercion, demo_ref_vs_smart,
};

fn main() {
    let arg = std::env::args().nth(1);
    let mode = arg.as_deref().unwrap_or("full");

    if mode == "guards" {
        println!("=== 15.2 §四 MutexGuard + Deref ===");
        demo_mutex_guard();
        println!("\nok: guards demo 完成");
        return;
    }

    if mode == "nested" {
        println!("=== 15.2.1 多层 coercion 链 ===");
        demo_nested_coercion();
        println!("\nok: nested demo 完成");
        return;
    }

    if mode == "mut" {
        println!("=== 15.2.1 DerefMut + Box move-out ===");
        demo_deref_mut();
        println!("\nok: mut demo 完成");
        return;
    }

    if mode == "ref-target" {
        println!("=== 15.2.3 普通引用 vs 智能指针 ===");
        demo_ref_vs_smart();
        println!("\n=== 15.2.3 关联类型 type Target ===");
        demo_associated_type();
        println!("\nok: ref-target demo 完成");
        return;
    }

    if mode == "dot-paths" {
        println!("=== 15.2.2 点号两条路线 ===");
        demo_dot_two_paths();
        println!("\nok: dot-paths demo 完成");
        return;
    }

    println!("=== 15.2 基础：* / deref / coercion ===");
    demo_basic();

    println!("\n=== 15.2 §三 方法调用自动解引用 ===");
    demo_method_call();

    println!("\nok: deref demo 完成（-- guards | -- nested | -- mut | -- ref-target | -- dot-paths）");
}
