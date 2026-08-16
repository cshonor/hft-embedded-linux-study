use nomicon_06_obrm_raii::{construct_drop, forget_leak, panic_guard, poisoning, proxy_types};

fn main() {
    if std::env::args().any(|a| a == "--drop-order") {
        println!("=== 1 recursive Drop order ===");
        construct_drop::recursive_drop_order();
        println!("=== ManuallyDrop ===");
        construct_drop::manually_drop_skip();
        return;
    }

    println!("=== 1 Option::take ===");
    println!("taken = {:?}", construct_drop::option_take_breaks_field_drop());

    println!("=== 2 mem::forget ===");
    forget_leak::forget_box();
    println!("{}", forget_leak::forget_is_safe_but_leaks());

    println!("=== 3 forget Drain (leak amplification) ===");
    println!("vec = {:?}", proxy_types::forget_drain());
    println!("{}", proxy_types::proxy_warning());

    println!("=== 4 panic guard ===");
    let mut slot = Some(String::from("before"));
    println!("prev = {}", panic_guard::guarded_operation(&mut slot));
    println!("slot = {:?}", slot);

    println!("=== 5 Mutex poison ===");
    println!("poisoned = {}", poisoning::mutex_poisoned_after_panic());

    println!("\n(tip: cargo run -- --drop-order)");
}
