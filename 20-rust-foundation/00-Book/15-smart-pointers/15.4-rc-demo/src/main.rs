// 15.4 Rc demo
//   cargo run        — 共享尾链表 + strong_count
//   cargo run -- count — Rc<i32> 计数逐步

use rc_demo::{demo_rc_count_steps, demo_shared_tail_list};

fn main() {
    let arg = std::env::args().nth(1);
    let mode = arg.as_deref().unwrap_or("full");

    if mode == "count" {
        println!("=== 15.4 §二 Rc<i32> strong_count 逐步 ===\n");
        demo_rc_count_steps();
        println!("\nok: count demo 完成");
        return;
    }

    println!("=== 15.4 §三 共享尾链表 ===\n");
    demo_shared_tail_list();
    println!("\nok: rc demo 完成（-- count）");
}
