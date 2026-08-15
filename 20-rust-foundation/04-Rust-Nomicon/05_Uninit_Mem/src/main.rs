use nomicon_05_uninit_mem::{checked, drop_flags, maybe_uninit, ptr_ops};

fn main() {
    let drop_demo = std::env::args().any(|a| a == "--drop-flags");

    if drop_demo {
        println!("=== 2 drop flags (flag=true) ===");
        drop_flags::conditional_drop(true);
        println!("=== 2 drop flags (flag=false, no drop) ===");
        drop_flags::conditional_drop(false);
        println!("=== 2 reassignment ===");
        drop_flags::reassignment_drops_old();
        return;
    }

    println!("=== 1 checked init ===");
    println!("conditional_init(true) = {}", checked::conditional_init(true));
    println!("move -> {}", checked::move_leaves_uninit());

    println!("=== 3 MaybeUninit array ===");
    println!("array = {:?}", maybe_uninit::init_array());

    println!("=== 3 Vec set_len ===");
    println!("vec = {:?}", maybe_uninit::vec_from_uninit_slots());

    println!("=== 3 ptr write/copy ===");
    println!("dst, overlap = {:?}", ptr_ops::write_and_copy());

    println!("\n(tip: cargo run -- --drop-flags)");
}
