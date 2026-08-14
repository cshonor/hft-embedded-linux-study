// 15.5 RefCell demo
//   cargo run           — Rc<RefCell> 共享尾链表
//   cargo run -- cell   — Rc<Cell<i32>>
//   cargo run -- refcell — Rc<RefCell<String>>

use refcell_demo::{demo_rc_cell, demo_rc_refcell_list, demo_rc_refcell_string};

fn main() {
    let arg = std::env::args().nth(1);
    let mode = arg.as_deref().unwrap_or("full");

    if mode == "cell" {
        println!("=== 15.5 §三 Rc<Cell<i32>> ===\n");
        demo_rc_cell();
        println!("\nok: cell demo 完成");
        return;
    }

    if mode == "refcell" {
        println!("=== 15.5 §四 Rc<RefCell<String>> ===\n");
        demo_rc_refcell_string();
        println!("\nok: refcell demo 完成");
        return;
    }

    println!("=== 15.5 §四 Rc<RefCell> 共享尾链表 ===\n");
    demo_rc_refcell_list();
    println!("\nok: refcell demo 完成（-- cell | -- refcell）");
}
