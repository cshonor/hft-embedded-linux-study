use nomicon_07_concurrency_atomic::{atomics, data_races, send_sync};

fn main() {
    println!("=== 1 data races / safe access ===");
    println!("safe_get = {:?}", data_races::demo_safe_shared());
    println!("{}", data_races::race_condition_is_safe_but_wrong());

    println!("=== 2 Send / Sync ===");
    send_sync::compile_time_bounds();
    send_sync::sync_means_shared_ref_is_send();
    println!("{}", send_sync::non_thread_safe_types_note());

    println!("=== 3 atomics ===");
    println!("Relaxed count (4×1000) = {}", atomics::relaxed_counter(4));
    println!("Release/Acquire payload = {}", atomics::release_acquire_publish());
    println!("SeqCst count (4×1000) = {}", atomics::seq_cst_counter(4));
}
