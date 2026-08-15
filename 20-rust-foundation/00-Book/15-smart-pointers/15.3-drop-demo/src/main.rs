// 15.3 Drop demo
//   cargo run           — 作用域 + LIFO 顺序
//   cargo run -- custom — FileHandle：手写 drop + 字段回收
//   cargo run -- early  — std::mem::drop
//   cargo run -- nested — Outer/Inner + Vec 正序
//   cargo run -- guard  — MutexGuard RAII
//   cargo run -- manual — ManuallyDrop
//   cargo run -- lifo   — Tag self.0 + LIFO 顺序
//   cargo run -- socket  — 15.3.2 TcpSocket / Conn / move
//   cargo run -- obrm    — 15.3.0 所有者 Drop vs 借用

use drop_demo::{
    demo_custom_drop_then_fields, demo_manually_drop, demo_mem_drop_early, demo_mutex_guard_drop,
    demo_nested_drop, demo_obrm_borrow_vs_owner, demo_scope_and_order, demo_socket_all,
    demo_tag_lifo, demo_vec_drop_order,
};

fn main() {
    let arg = std::env::args().nth(1);
    let mode = arg.as_deref().unwrap_or("full");

    if mode == "custom" {
        println!("=== 15.3 §二 手写 Drop → 字段自动回收 ===\n");
        demo_custom_drop_then_fields();
        println!("\nok: custom demo 完成");
        return;
    }

    if mode == "early" {
        println!("=== 15.3 §三 mem::drop 提前释放 ===\n");
        demo_mem_drop_early();
        println!("\nok: early demo 完成");
        return;
    }

    if mode == "nested" {
        println!("=== 15.3.1 嵌套 + Vec drop 顺序 ===\n");
        demo_nested_drop();
        println!();
        demo_vec_drop_order();
        println!("\nok: nested demo 完成");
        return;
    }

    if mode == "guard" {
        println!("=== 15.3 §四 MutexGuard RAII ===\n");
        demo_mutex_guard_drop();
        println!("\nok: guard demo 完成");
        return;
    }

    if mode == "lifo" {
        println!("=== 15.3.1 Tag + self.0 · LIFO ===\n");
        demo_tag_lifo();
        println!("\nok: lifo demo 完成");
        return;
    }

    if mode == "socket" {
        println!("=== 15.3.2 Drop 与网络 Socket RAII ===\n");
        demo_socket_all();
        println!("\nok: socket demo 完成");
        return;
    }

    if mode == "obrm" {
        println!("=== 15.3.0 OBRM：借用不 Drop，所有者出作用域才释放 ===\n");
        demo_obrm_borrow_vs_owner();
        println!("\nok: obrm demo 完成");
        return;
    }

    if mode == "manual" {
        println!("=== 15.3.1 ManuallyDrop ===\n");
        demo_manually_drop();
        println!("\nok: manual demo 完成");
        return;
    }

    println!("=== 15.3 作用域自动 drop + LIFO ===\n");
    demo_scope_and_order();
    println!("\nok: drop demo 完成（-- obrm | -- lifo | -- custom | -- socket | -- early | -- nested | -- guard | -- manual）");
}
