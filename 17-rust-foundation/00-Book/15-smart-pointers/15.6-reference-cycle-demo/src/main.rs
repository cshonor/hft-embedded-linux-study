mod node_cycle;
mod ref_cycle;
mod weak_tree;

fn main() {
    let arg = std::env::args().nth(1);
    let mode = arg.as_deref().unwrap_or("full");

    if mode == "node" {
        println!("=== 15.6 §二 Rc 节点环（泄漏）===\n");
        node_cycle::demo_node_cycle_leak();
        println!("\n=== 15.6 §三 Weak 修复 ===\n");
        node_cycle::demo_node_weak_fix();
        println!("\nok: node demo 完成");
        return;
    }

    println!("=== 15.6 §四 List 引用环（15-26）===\n");
    ref_cycle::run();

    println!("\n=== 15.6 §三 Weak 父指针树（15-29）===\n");
    weak_tree::run();

    println!("\nok: cycle demo 完成（-- node）");
}
